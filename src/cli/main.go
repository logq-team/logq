package main

import (
	"flag"
	"fmt"
	"os"

	"logq/src/internal/engine"
)

func main() {
	fmt.Printf("[DEBUG] " + "Hello from Go:main()\n")
	text := flag.String("text", "hello from Go", "text to pass to C++")
	number := flag.Int64("number", 21, "integer to double in C++")
	flag.Parse()
	if flag.NArg() != 0 {
		fmt.Fprintln(os.Stderr, "unexpected positional arguments")
		flag.Usage()
		os.Exit(2)
	}

	result, err := engine.Demo(*text, *number)
	if err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	fmt.Printf("Go -> C++: text=%q, number=%d\n", *text, *number)
	fmt.Printf("C++ -> Go: bytes=%d, newlines=%d, doubled=%d, __cplusplus=%d\n",
		result.Bytes, result.Newlines, result.Doubled, result.CPPStandard)
}
