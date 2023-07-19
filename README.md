# My_Shell
NAME mysh – run a command line interpreter

DESCRIPTION

mysh invokes a command line interpreter that supports command execution and input/output redirection. mysh takes a single optional command line argument that specifies the prompt string. If no arguments are given, the prompt defaults to “mysh: “. If “-“ is given, do not print a prompt.


OPERATORS

On an input command line, commands, operators and operands are always separated by whitespace. mysh supports the following command line operators:

'&'

Place commands into the background: after invoking the specified commands, mysh immediately prints its prompt and waits for another command line. ‘&’ may only be specified as the final command line argument.

'<'

Redirect the current command’s standard input stream from the file named immediately after the ‘<’ operator.  
Redirect the current command’s standard output stream to the file named immediately after the ‘>’ operator. mysh never “clobbers” an existing output file.  
Redirect the current command’s standard output stream to the file named immediately after the ‘>>’ operator. Append to the file if it already exists.

'|'

Redirect the current command’s standard output stream to the standard input stream of the succeeding command. There may be any number of pipe-connected processes.

EXITING

mysh exits with 0 when the user inputs “exit” or CNTL-D. These are the only circumstances under which mysh exits. When command lines are malformed or fail to execute, mysh prints an appropriate error message then prompts the user for another command line.
