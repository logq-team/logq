package cmd

import (
	"errors"
)

type Command string

const (
	HelpCommand     Command = "--help"
	GrepCommand     Command = "--grep"
	OverviewCommand Command = "--overview"
)

const (
	ProgramName = "logq"
)

func ProcessArgs(args []string) (Command, error) {
	// у нас количество аргументов всегда > 1
	if len(args) == 1 {
		return "", errors.New("Type \"" + ProgramName + " --help\" for more information\n")
	}

	commandName := args[1]
	switch commandName {
	case string(HelpCommand):
		return HelpCommand, nil
	case string(GrepCommand):
		//еще минимум 2 аргумента должно быть
		//паттерн и путь к архиву
		//we also need minimum 2 args
		//pattern and archive name
		if len(args) < 4 {
			return "", errors.New("Not enough arguments\n\"grep\" command usage:\n" +
				ProgramName + " --grep [PATTERN] [PATH_TO_ARCHIVE]")
		}

		return GrepCommand, nil
	case string(OverviewCommand):
		//еще минимум 1 аргумент должен быть
		//we also need minimum 1 arg
		if len(args) < 3 {
			return "", errors.New("Not enough arguments\n\"overview\" command usage:\n" +
				ProgramName + " --overview [PATH_TO_ARCHIVE]")
		}

		return OverviewCommand, nil
	default:
		if len(commandName) >= 2 && commandName[:2] == "--" {
			return "", errors.New("Unknown flag\n" +
				"Type \"" + ProgramName + " --help\" for more information\n")
		} else {
			return "", errors.New("Invalid usage\n" +
				"Type \"" + ProgramName + " --help\" for more information\n")
		}

	}
}
