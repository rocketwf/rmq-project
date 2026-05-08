all: build run
    
# Run the input-generator program and write to input/{n}.in
generate_input:
    cd input-generator && cargo run -- -n 1000,3000,10000,30000,100000,300000,1000000,3000000,10000000 --output ../input

build:
    cd rmq-rust && cargo build --release
    cp rmq-rust/target/release/rmq-rust rmq

run:
    ./rmq input > data.csv
