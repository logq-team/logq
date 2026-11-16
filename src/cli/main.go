package main

import (
	"flag"
	"fmt"
	"logq/src/internal/engine"
)

func main() {

	overviewFlag := flag.Bool("overview", false, "overview is used for common statistics")
	//grepFlag := flag.String("grep", "all", "how to process logs")

	flag.Parse()
	args := flag.Args()



	if !*overviewFlag || len(args) != 1 {
		fmt.Printf("Невалидное использование утилиты")
	}

	err := engine.ProcessArchive(args[0])
	if err != nil {
		println("Err happened while processing\n", err.Error())
	}
}
