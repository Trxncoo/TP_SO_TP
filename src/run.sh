export INSCRICAO=300
export NPLAYERS=2
export DURACAO=300
export DECREMENTO=10




cd ..

make clean

make

gnome-terminal  -- bash -c "./motor; exit"
sleep 0.5
gnome-terminal  -- bash -c "./jogoUI nome1; exit"
sleep 0.1