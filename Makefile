all: check check_ansi check_misra test breaks example/encodex

encodex.c:
encodex.h:
test/test.c:
example/app.c:
docs/visualizer.c:
break/chosen_plaintext.c:
break/known_plaintext.c:
break/mitm_validator.c:

KEY=0102030405060708091011121314151617181920212223242526272829303132

check_ansi: encodex.c encodex.h
	cppcheck encodex.h encodex.c -DENCODEX_CHECK --enable=all --inconclusive --check-level=exhaustive --inline-suppr --suppress=missingIncludeSystem --suppress=unmatchedSuppression --error-exitcode=1 --std=c90 --quiet

check_misra: encodex.c encodex.h
	cppcheck encodex.h encodex.c -DENCODEX_CHECK --enable=all --inconclusive --check-level=exhaustive --inline-suppr --suppress=missingIncludeSystem --suppress=unmatchedSuppression --error-exitcode=1 --std=c90 --addon=misra --quiet

check: encodex.c encodex.h
	$(CC) -c encodex.c -o encodex.o -ansi -Wall -Werror -pedantic -Os
	size encodex.o

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

breaks: break/chosen_plaintext break/known_plaintext break/mitm_validator
	break/chosen_plaintext
	break/known_plaintext
	break/mitm_validator

docs-images: docs/visualizer
	docs/visualizer all
	openssl enc -aes-256-ecb -nopad -nosalt -K $(KEY) -in example/portrait.data -out /tmp/encodex_portrait_aes.data
	docs/visualizer /tmp/encodex_portrait_aes.data docs/portrait_aes.png 300
	openssl enc -aes-256-ecb -nopad -nosalt -K $(KEY) -in example/teapot.data -out /tmp/encodex_teapot_aes.data
	docs/visualizer /tmp/encodex_teapot_aes.data docs/teapot_aes.png 300
	openssl enc -des-ecb -nopad -nosalt -K 0102030405060708 -provider default -provider legacy -in example/portrait.data -out /tmp/encodex_portrait_des.data
	docs/visualizer /tmp/encodex_portrait_des.data docs/portrait_des.png 300
	openssl enc -des-ecb -nopad -nosalt -K 0102030405060708 -provider default -provider legacy -in example/teapot.data -out /tmp/encodex_teapot_des.data
	docs/visualizer /tmp/encodex_teapot_des.data docs/teapot_des.png 300

test/test: test/test.c encodex.c encodex.h
	$(CC) test/test.c -o test/test -I. -ansi -Wall -Werror -pedantic

example/encodex: example/app.c encodex.c encodex.h
	$(CC) example/app.c encodex.c -o example/encodex -I. -ansi -Wall -Werror -pedantic

break/chosen_plaintext: break/chosen_plaintext.c encodex.c encodex.h
	$(CC) break/chosen_plaintext.c encodex.c -o break/chosen_plaintext -I. -ansi -Wall -Werror -pedantic

break/known_plaintext: break/known_plaintext.c encodex.c encodex.h
	$(CC) break/known_plaintext.c encodex.c -o break/known_plaintext -I. -ansi -Wall -Werror -pedantic

break/mitm_validator: break/mitm_validator.c encodex.c encodex.h
	$(CC) break/mitm_validator.c encodex.c -o break/mitm_validator -I. -ansi -Wall -Werror -pedantic

docs/visualizer: docs/visualizer.c encodex.c encodex.h
	$(CC) docs/visualizer.c -o docs/visualizer -I. -ansi -Wall -Werror -pedantic

clean:
	rm -rf encodex.o test/test example/encodex
	rm -rf break/chosen_plaintext break/known_plaintext break/mitm_validator
	rm -rf docs/visualizer
	rm -rf example/portrait_encoded.data example/portrait_decoded.data
	rm -rf example/portrait_encoded_cbc.data example/portrait_decoded_cbc.data
	rm -rf example/teapot_encoded.data example/teapot_decoded.data
	rm -rf example/teapot_encoded_cbc.data example/teapot_decoded_cbc.data
