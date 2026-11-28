package main

import (
	"fmt"
	"logq/src/internal/cmd"
	"logq/src/internal/engine"
	"logq/src/internal/engine/overview"
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
	case cmd.HelpCommand:
		fmt.Println(engine.HandleHelp())
	case cmd.GrepCommand:
		err := engine.HandleGrep(args[2], args[3])
		fmt.Println(err.Error())
	case cmd.OverviewCommand:
		reqPntr, err := overview.PrepareRequestForOverview(args[2:])
		if err != nil {
			fmt.Println(err.Error())
			break
		}

		output, err := overview.HandleOverview(reqPntr)
		if err != nil {
			fmt.Println(err.Error())
		} else {
			fmt.Println(output)
		}
	default:
		fmt.Println("Unknown flag")
	}
}
