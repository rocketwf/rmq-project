.PHONY: all generate_input build build-rust run plot open-plots

all: build run plot open-plots

# Run the input-generator program and write to input/{n}.in
generate_input:
	cd input-generator && cargo run -- -n 1000,3000,10000,30000,100000,300000,1000000,3000000,10000000 --output ../input

# TODO: Customize this for your language.
# Make sure to end with a `rmq` binary in the root directory.
build: build-rust

build-rust:
	cd rmq-rust && cargo build --release
	cp rmq-rust/target/release/rmq-rust rmq

run:
	./rmq input > data.csv

plot:
	python3 plot.py

open-plots:
	open plots/*.png
