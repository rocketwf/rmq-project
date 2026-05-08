import Control.Monad (forM_, replicateM)
import Data.List (sortBy)
import Data.Ord (comparing)
import Data.Time.Clock.System (getSystemTime, systemToTAITime)
import Data.Time.Clock.TAI (diffAbsoluteTime)
import Data.Word (Word64)
import System.Environment (getArgs)
import System.Exit (exitFailure)
import System.IO (hPrintf, hPutStrLn, stderr)
import qualified Data.Vector.Unboxed as V
import qualified System.Directory as Dir

-- RMQ typeclass. Instances also provide:
--   name    :: String
--   maxN    :: Int          -- optional; defaults to maxBound
--   build   :: V.Vector Word64 -> rmq
--   space   :: rmq -> Int
--   query   :: rmq -> Int -> Int -> Word64

-- Trivial implementation that computes each query on the fly.
newtype Naive = Naive (V.Vector Word64)

naiveName :: String
naiveName = "QuadraticQuery"

-- NOTE: Do not use this for the improved implementations!
naiveMaxN :: Int
naiveMaxN = 10000

naiveBuild :: V.Vector Word64 -> Naive
naiveBuild = Naive

naiveSpace :: Naive -> Int
naiveSpace (Naive v) = 8 + 16 + V.length v * 8

naiveQuery :: Naive -> Int -> Int -> Word64
naiveQuery (Naive v) l r = V.minimum (V.slice l (r - l + 1) v)

-- -------------------------------------------------------------
-- TODO: Add further implementations here.
-- -------------------------------------------------------------

data Input = Input
    { inputData   :: !(V.Vector Word64)
    , inputQueryL :: !(V.Vector Int)
    , inputQueryR :: !(V.Vector Int)
    }

readInput :: FilePath -> IO Input
readInput file = do
    ws <- words <$> readFile file
    let (n : q : rest0) = ws
        n' = read n; q' = read q
        (ds, rest1) = splitAt n' rest0
        qs = rest1
        lrs = chunksOf2 q' qs
    return Input
        { inputData   = V.fromList (map read ds)
        , inputQueryL = V.fromList (map fst lrs)
        , inputQueryR = V.fromList (map snd lrs)
        }
  where
    chunksOf2 0 _        = []
    chunksOf2 k (a:b:xs) = (read a, read b) : chunksOf2 (k-1) xs
    chunksOf2 _ _        = []

bench :: String -> Int -> Int -> (Int -> Int -> Word64) -> Input -> IO ()
bench name maxN spaceBytes queryFn input = do
    let n = V.length (inputData input)
        q = V.length (inputQueryL input)
    hPrintf stderr "%10d\t%20s\t" n name
    if n > maxN
        then hPutStrLn stderr "skipped"
        else do
            hPrintf stderr "%10d\t" spaceBytes
            t0 <- getSystemTime
            let s = V.foldl' (+) 0 $
                        V.zipWith queryFn (inputQueryL input) (inputQueryR input)
            s `seq` return ()
            t1 <- getSystemTime
            let elapsed = diffAbsoluteTime (systemToTAITime t1) (systemToTAITime t0)
                ns = fromRational (toRational elapsed) * 1e9 :: Double
                nsPerQuery = ns / fromIntegral q
            putStrLn $ show n ++ "," ++ show q ++ "," ++ name ++ "," ++
                       show spaceBytes ++ "," ++ show s ++ "," ++
                       show nsPerQuery
            hPrintf stderr "%3d\t%.2fns/q\n" (s `mod` 1000) nsPerQuery

benchNaive :: Input -> IO ()
benchNaive input =
    let rmq@(Naive _) = naiveBuild (inputData input)
    in bench naiveName naiveMaxN (naiveSpace rmq) (naiveQuery rmq) input

-- TODO: Add bench helpers for other implementations here.

main :: IO ()
main = do
    args <- getArgs
    case args of
        [] -> hPutStrLn stderr "Usage: rmq-haskell <input_dir>" >> exitFailure
        (fileOrDir : _) -> do
            putStrLn "n,q,name,space,sum,time"
            hPutStrLn stderr $ "Reading input from \"" ++ fileOrDir ++ "\" .."
            isFile <- Dir.doesFileExist fileOrDir
            inputs <-
                if isFile
                    then (:[]) <$> readInput fileOrDir
                    else do
                        entries <- Dir.listDirectory fileOrDir
                        let inFiles = filter (\f -> drop (length f - 3) f == ".in") entries
                        ins <- mapM (\f -> readInput (fileOrDir ++ "/" ++ f)) inFiles
                        return $ sortBy (comparing (V.length . inputData)) ins
            forM_ inputs $ \input -> do
                benchNaive input
                -- TODO: Add other implementations here.
