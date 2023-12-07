export INSCRICAO=20
export NPLAYERS=3
export DURACAO=300
export DECREMENTO=10




cd ..

make clean

make

gnome-terminal  --full-screen -- bash -c "./motor; exit"
sleep 0.5
gnome-terminal  --full-screen -- bash -c "./jogoUI nome1; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI nome2; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI nome3; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI nome4; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI nome5; exit"