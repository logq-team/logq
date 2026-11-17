package main

import (
	"fmt"
	"logq/src/internal/cmd"
	"os"
)

func main() {
	args := os.Args

	commandType, err := cmd.ProcessArgs(args)

	if err != nil {
		fmt.Println(err.Error())
		return
	}

	switch commandType {
	case cmd.HelpCommand
	}
}
