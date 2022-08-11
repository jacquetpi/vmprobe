mkdir .build
cd .build
cmake ..
make
mv vmprobe ../
cd ..
sudo ./vmprobe
echo ""