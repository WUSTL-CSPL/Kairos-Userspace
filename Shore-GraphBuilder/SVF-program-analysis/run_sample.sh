# cd Release-build
# opt -f -enable-new-pm=0 -load tools/Rain/librain.so -rainPass ../toy_examples/llvm13_bitcode.bc -o binary
opt -f -enable-new-pm=0 -load tools/Rain/librain.so -profiling ../toy_examples/swap.ll -o binary
# opt -f -enable-new-pm=0 -load tools/Rain/librain.so -profiling ~/catkin_ws/build/amcl/onewhole.bc -o binary 

# ./bin/wpa /home/ub1804/anytime-dfi_ws/slack-time/SVF/toy_examples/swap.ll -ander  --dump-pag --print-all-pts
#./bin/dvf -cxt -query=all  /home/ub1804/anytime-dfi_ws/slack-time/SVF/toy_examples/swap.ll --dump-vfg
#./bin/cfl  --dump-vfg --cflsvfg /home/ub1804/anytime-dfi_ws/slack-time/SVF/toy_examples/swap.ll --dump-pag