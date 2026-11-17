package cmd

import "errors"

type Command string

const (
	HelpCommand     Command = "help"
	GrepCommand     Command = "grep"
	OverviewCommand Command = "overview"
)

func ProcessArgs(args []string) (Command, error) {
	programName := args[0]
	// у нас количество аргументов всегда >= 1
	if len(args) == 1 {
		return "", errors.New("Type \"" + programName + " help\" for more information\n")
	}

	commandName := args[1]
	switch commandName {
	case string(HelpCommand):
		return HelpCommand, nil
	case string(GrepCommand):
		//еще минимум 2 аргумента должно быть
		if len(args) < 4 {
			return "", errors.New("\"grep\" command usage:\n" +
				programName + " grep [PATTERN] [PATH_TO_ARCHIVE]")
		}

		return GrepCommand, nil
	case string(OverviewCommand):
		//еще минимум 1 аргумент должен быть
		if len(args) < 4 {
			return "", errors.New("\"overview\" command usage:\n" +
				programName + " overview [PATH_TO_ARCHIVE]")
		}

		return OverviewCommand, nil
	default:
		return "", errors.New("Unknown command\n" +
			"Type \"" + programName + " help\" for more information\n")
	}
}
