#/bin/bash
cp *.spec *.changes *.tar.gz ~/src/home:vranki/libsiilihai
cp ../../libsiilihai_*.tar.gz ../../libsiilihai*.dsc ~/src/home:vranki/libsiilihai
pushd ~/src/home:vranki/libsiilihai
osc commit -m 'new stuff'
popd

