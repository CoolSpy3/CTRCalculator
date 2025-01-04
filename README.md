# CoolSpy3's Terminal-based RPN Calculator
A terminal-based [RPN](https://en.wikipedia.org/wiki/Reverse_Polish_notation) calculator.

## Basic Syntax and Interface
The calculator presents an interface via a terminal prompt.
All whitespace is ignored.
Newlines can be escaped by appending a `\` to the end of a line.
This will cause the calculator to wait for an additional line (either on the command line or in a file) before the command is processed.
Comments can also be added with the `#` character.
Any text on a line after a `#` is ignored.

Most operations in the calculator operate on a list of numbers called the stack.
This stack is printed out before each user prompt.
The rightmost number is the most recent value on the stack.
Numbers can be pushed onto the end of the stack by entering them in their own command.
(e.x. Typing `3` will push `3` onto the stack.)
Numbers can be removed from the stack by various [commands](#standard-commands).

## Configuration
When the program starts, it will attempt to read a file named `.calcrc`.
Each line of this file will be executed as if it was passed to stdin.
This allows the user to perform any setup (such as defining custom aliases) before every session.
If any command in the configuration file encounters an error, execution of the entire file will be stopped.
Similarly, if the config file executes `\exit`, the program will terminate.

On POSIX systems, this file is located in `$XDG_CONFIG_HOME`.
If `$XDG_CONFIG_HOME` is not defined, the file is located in `$HOME`.
If `$HOME` is also not defined, the file is located in the user's home directory.

On Windows, this file is located in the user's profile directory.

On all systems, if the file does not exist, it will be created.

## Command Chaining
Commands can be chained with the `;` character.
If any command in a command chain fails or invokes `\exit`, the entire chain immediately fails or the program exits.

Ending any command in a chain with `\` is unsupported.

## Evaluating Expressions
Several parts of the calculator accept expressions.
Often this is within the context of brackets (either `[<expression>]` or `{<expression>}`).
In this case, the behavior is identical to if the expression was passed as an input to the calculator.
This means that alias definitions and command chains in expressions are allowed.
The "result" of the expression is then defined as the most recently pushed value on the stack after the expression is executed.
If required, this value will be automatically popped and used in the requested operation.
(This means that both `ans={!!}` and `ans={}` will set `ans` equal to the last value in the stack, **but the latter will also remove that value from the stack**.)

Ending an expression with `\` is not supported.

## Aliases
Aliases can be defined using the `=` operator. When an input *exactly matches* a defined alias (ignoring whitespace), the entire input is replaced with the alias's value.
This can be combined with command chaining to create more complex aliases (ex. `\circarea=2;\pow;pi;*`).
Keep in mind that, by default, the `=` operator is greedy and will consume the rest of the line.
This means that the line `pi=3.141;e=2.718` will define `pi` as being equal to `3.141;e=2.718`.
To override this behavior, parentheses can be used: `pi=(3.141);e=2.718`.
It is also possible to set a variable to the result of an expression by using curly brackets (e.x. `pi={22/7}`).
This will evaluate `22/7` and set `pi` equal to the result. (*See [evaluating expressions](#evaluating-expressions)*)
This is different from `pi=22/7` or `pi=(22/7)`, which will set `pi` equal to the string `22/7`, which will then be evaluated when the alias is used.

Using `()` or `{}` outside of alias definitions is unsupported.

Aliases can be erased by setting them equal to the empty string.
Note that this does not mean that the alias becomes a no-op, but rather than the alias is undefined.
Thus, `pi=(3.14);pi=();pi` will result in an error because `pi` is no longer defined.
This means that if you accidentally type something like `1=2` (which is allowed), you can easily undo it by typing `1=`.

## Arithmetic Shorthand
Because this is an RPN calculator, most commands operate on values in the stack, however, in order to make simple operations more convenient, the calculator supports basic arithmatic in the form `[num1][+-*/][num2]`.
Statements of this form push `num1` and `num2` onto the stack and then execute the requested operation.
At this time, only numbers are supported.
This means that `num1` and `num2` are not evaluated as expressions and thus, cannot contain aliases or more complex functions.

Thus, `1/2` will evaluate to `.5`, but `pi=(3.1415);pi/2` is invalid.

## Dereferencing the Stack
Values on the stack can be pushed onto the stack by dereferencing them.
This is done with the `[<expression>]` syntax.
This evaluates `<expression>`, reads that index on the stack, and pushes the value onto the stack.
This operation does not remove the old value from the stack.
If `<expression>` evaluates to a floating point number, that number is rounded to the nearest integer.

The stack is zero-indexed, so `[0]` refers to the oldest value in the stack and `[<stack_size>-1]` refers to the newest value.
Additionally, negative numbers are supported in the same way as negative list indices in languages like Python.
(e.g. `[-1]` refers to the most recent value and `[-<stack_size>]` refers to the oldest.)

For convenience, the `!!` shorthand will copy the most recent value in the stack.

## Standard Commands
The standard commands define the actual operations this calculator is able to perform.
Unless otherwise specified, these commands pop the most recent value(s) from the stack and push their result onto the end of the stack.

In order to better distinguish these functions from aliases, I've prefixed many of them with `\`.
However, because I don't want to type it, if a line would otherwise fail, the calculator will currently attempt to prefix `\`, so both the `\` and non-`\` variants will be considered valid.
In the future, I hope to add a toggle for this feature.

| Command    | Action                                                                       |
|------------|------------------------------------------------------------------------------|
| `\exit`    | Exits the program.                                                           |
| `+`        | Adds the two most recent numbers in the stack.                               |
| `-`        | Subtracts the two most recent numbers in the stack.                          |
| `*`        | Multiplies the two most recent numbers in the stack.                         |
| `/`        | Divides the two most recent numbers in the stack.                            |
| `\pow`     | Raises the second most recent number in the stack to the power of the first. |
| `1/`       | Takes the multiplicative inverse of the most recent value in the stack.      |
| `\clear`   | Clears the stack.                                                            |
| `\swap`    | Swaps the two most recent values in the stack.                               |
| `\roll`    | Moves the oldest value in the stack to the most recent position.             |
| `\drop`    | Drops the most recent value in the stack.                                    |
| `\aliases` | Prints all defined aliases.                                                  |
| `\sqrt`    | Takes the square root of the most recent value in the stack.                 |
| `\cbrt`    | Takes the cube root of the most recent value in the stack.                   |
| `\sin`     | Takes the sine of the most recent value in the stack.                        |
| `\cos`     | Takes the cosine of the most recent value in the stack.                      |
| `\tan`     | Takes the tangent of the most recent value in the stack.                     |
| `\asin`    | Takes the inverse sine of the most recent value in the stack.                |
| `\acos`    | Takes the inverse cosine of the most recent value in the stack.              |
| `\atan`    | Takes the inverse tangent of the most recent value in the stack.             |
| `\sinh`    | Takes the hyperbolic sine of the most recent value in the stack.             |
| `\cosh`    | Takes the hyperbolic cosine of the most recent value in the stack.           |
| `\tanh`    | Takes the hyperbolic tangent of the most recent value in the stack.          |
| `\asinh`   | Takes the inverse hyperbolic sine of the most recent value in the stack.     |
| `\acosh`   | Takes the inverse hyperbolic cosine of the most recent value in the stack.   |
| `\atanh`   | Takes the inverse hyperbolic tangent of the most recent value in the stack.  |
| `\log`     | Takes the log base e of the most recent value in the stack.                  |
| `\log2`    | Takes the log base 2 of the most recent value in the stack.                  |
| `\log10`   | Takes the log base 10 of the most recent value in the stack.                 |
| `\exp`     | Raises Euler's number to the power of the most recent value in the stack.    |
