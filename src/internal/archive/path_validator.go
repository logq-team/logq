package archive

import "strings"

const (
	ParentStart = "../"
	ParentEnd   = "/.."
)

// валидация строки
// чего не должно быть в строке:
// - .. - родительского каталога
func ValidatePath(path string) bool {
	pathLen := len(path)
	parentEndLen := len(ParentEnd)
	parentStartLen := len(ParentStart)
	noParentDir := strings.Contains(path, "/../") ||
		path == ".." ||
		(pathLen >= parentEndLen && path[pathLen-parentEndLen:] == ParentEnd) ||
		(pathLen >= parentStartLen && path[:parentStartLen] == ParentStart)

	return !noParentDir
}
