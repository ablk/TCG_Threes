https://github.com/ablk/TCG_HW.git

## build
make

## train & save new weights
threes --total=300000 --block=1000 --limit=1000 --play="init=0 save=weights.bin alpha=0.003125"

## load pre-trained weights and train
threes --total=2000000 --block=1000 --limit=1000 --play="load=weights_2000000p.bin save=weights.bin alpha=0.003125"

## play & save to txt
threes --total=1000 --play="load=weights_4000000p.bin alpha=0" --save="stat.txt"