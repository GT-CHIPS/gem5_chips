##################################
# Example Script for Running BFS benchmark
# for 10K instructions on 16c CHIPS system
#
# Written By: Tushar Krishna (tushar@ece.gatech.edu)
# Last Updated: Feb 5, 2019
##################################

## How to run
#./my_scripts/run_demo.sh "<additional_options>"

# Examples to try:
#./my_scripts/run_demo.sh "--interposer-link-width=32"
#./my_scripts/run_demo.sh "--interposer-link-width=8"
#./my_scripts/run_demo.sh "--interposer-link-width=32 --clip-logic-ifc-delay=12"


### RISCV SE
cpu_type=TimingSimpleCPU

# Cache Parameters
# L1
l1_size='16kB'
l1_assoc=4

# Total L2 = 2MB
# L2 per tile:
l2_size='128kB'
l2_assoc=8


./build/RISCV_MESI_Two_Level/gem5.opt \
-d my_outdir \
configs/example/se.py \
--cpu-type ${cpu_type} \
--num-cpus=16 \
--l1d_size=${l1_size} --l1i_size=${l1_size} --l1d_assoc=${l1_assoc} \
--num-l2caches=16 \
--l2_size=${l2_size} --l2_assoc=${l2_assoc} \
--num-dirs=4 \
--ruby --mem-size=4096MB \
--network=garnet2.0 \
--topology=CHIPS_Multicore_MemCtrlChiplet4 \
--mesh-rows=4 \
-c my_benchmarks/ligra/bin/riscv/BFS -o '-n 16 my_benchmarks/ligra/input/rMatGraph_J_5_100' \
--maxinsts=10000 \
$1


# Print Stats
echo
echo "CHIPS Demo: Stats:"
grep "sim_ticks" my_outdir/stats.txt
grep "average_packet_latency" my_outdir/stats.txt
