all: check check_ansi check_misra test example/encodex

encodex.c:
encodex.h:
test/test.c:
example/app.c:

KEY=0102030405060708091011121314151617181920212223242526272829303132

check_ansi:
	cppcheck encodex.h encodex.c --enable=all --suppress=missingIncludeSystem --suppress=staticFunction --std=c90 --quiet

check_misra:
	cppcheck encodex.h encodex.c --enable=all --suppress=missingIncludeSystem --suppress=staticFunction --std=c90 --addon=misra --quiet

check: encodex.c encodex.h
	$(CC) -c encodex.c -o encodex.o -ansi -Wall -Werror -pedantic

test: test/test example/encodex
	test/test
	example/encodex encode example/portrait.data example/portrait_encoded.data $(KEY)
	example/encodex decode example/portrait_encoded.data example/portrait_decoded.data $(KEY)
	example/encodex encode cbc example/portrait.data example/portrait_encoded_cbc.data $(KEY)
	example/encodex decode cbc example/portrait_encoded_cbc.data example/portrait_decoded_cbc.data $(KEY)
	example/encodex encode example/teapot.data example/teapot_encoded.data $(KEY)
	example/encodex decode example/teapot_encoded.data example/teapot_decoded.data $(KEY)
	example/encodex encode cbc example/teapot.data example/teapot_encoded_cbc.data $(KEY)
	example/encodex decode cbc example/teapot_encoded_cbc.data example/teapot_decoded_cbc.data $(KEY)

test/test: test/test.c
	$(CC) test/test.c -o test/test -I. -ansi -Wall -Werror -pedantic

example/encodex:
	$(CC) example/app.c encodex.c -o example/encodex -I. -ansi -Wall -Werror -pedantic

clean:
	rm -rf encodex.o test/test example/encodex
	rm -rf example/portrait_encoded.data example/portrait_decoded.data
	rm -rf example/portrait_encoded_cbc.data example/portrait_decoded_cbc.data
	rm -rf example/teapot_encoded.data example/teapot_decoded.data
	rm -rf example/teapot_encoded_cbc.data example/teapot_decoded_cbc.data
