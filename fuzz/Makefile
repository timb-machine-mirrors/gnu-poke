all:	pk.dict afl_gcc_whitelist in

pk.dict:
	./generate_afl_dict.py ../src/pkl-tab.y > pk.dict

afl_gcc_whitelist:
	find ../src/ -name '*.c' | xargs realpath > afl_gcc_whitelist

in:
	mkdir -p collected reduced in
	for file in $$(find ../testsuite -name '*.pk'); do cp $${file} collected/; done
	# from https://stackoverflow.com/a/13062074
	find collected/ -name '*.pk' | xargs sed -i -r ':a; s%(.*)/\*.*\*/%\1%; ta; /\/\*/ !b; N; ba'
	$${AFL_PATH}/afl-cmin -i collected -o reduced -m none -- ../src/poke --no-init-file -L @@
	parallel --jobs 50% $${AFL_PATH}/afl-tmin -i reduced/{/} -o in/{/} -m none -- ../src/poke --no-init-file -L @@ ::: reduced/*

clean:
	rm -rf pk.dict afl_gcc_whitelist in collected reduced
