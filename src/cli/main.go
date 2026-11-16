package main

import (
	"flag"
	"fmt"
)

func main() {

	//overviewFlag := flag.Bool("overview", false, "overview is used for common statistics")
	//grepFlag := flag.String("grep", "all", "how to process logs")

	flag.Parse()
	args := flag.Args()

	for _, str := range args {
		fmt.Printf("%s ", str)
	}
	fmt.Println()

	if len(args) != 1 {
		fmt.Printf("Невалидное использование утилиты")
	}

}
