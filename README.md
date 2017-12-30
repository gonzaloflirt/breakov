# breakov

AudioUnit / VST instrument slicing sound files using markov chains.
Very much inspired by [Sector](http://kymatica.com/sector) for iOS.

## Build

```
git clone --recursive https://github.com/gonzalofirt/breakov.git
cd breakov
mkdir build
cd build
cmake .. -Dbreakov_jucer_FILE=../breakov.jucer -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
