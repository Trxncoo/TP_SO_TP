export INSCRICAO=10
export NPLAYERS=3
export DURACAO=300
export DECREMENTO=10




cd ..

make clean

make

gnome-terminal  --full-screen -- bash -c "./motor; exit"
sleep 0.5
gnome-terminal  --full-screen -- bash -c "./jogoUI João; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI Pedro; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI Gonçalo; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI António; exit"
sleep 0.1
gnome-terminal  --full-screen -- bash -c "./jogoUI Mário; exit"