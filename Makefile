.PHONY: all generate_input python_env build debug build-rust build-cpp debug-cpp build-java build-csharp build-go build-kotlin build-haskell run plot open-plots latex latex-debug

all: build run python_env plot

# Run the input-generator program and write to input/{n}.in
generate_input:
	mkdir -p input && cd input-generator && cargo run -- -n 1000,3000,10000,30000,100000,300000,1000000,3000000,10000000 --output ../input

python_env:
	@if [ ! -d ".venv" ]; then \
		python3 -m venv .venv && \
		.venv/bin/pip install -r reqs.txt; \
	fi

build: build-cpp
debug: debug-cpp

build-cpp:
	g++ -std=c++17 -O3 -march=native rmq-cpp/*.cpp -o rmq

debug-cpp:
	g++ -std=c++17 -g -O0 -Wall -march=native rmq-cpp/*.cpp -o rmq

run:
	./rmq input > data.csv

plot:
	.venv/bin/python plot.py

open-plots:
	open plots/*.png

report:
	cd report && latexmk -pvc -pdf -interaction=nonstopmode report.tex -cd -shell-escape
report-debug:
	cd report && latexmk -pdf report.tex -cd
