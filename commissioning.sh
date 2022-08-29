g++ commissioning-server.cpp -lpqxx -lpq -std=c++14 -o commissioning-server -static-libstdc++ -ljsoncpp -lcrypt
sudo service commissioning-server stop
sudo cp commissioning-server /bin/commissioning-server
sudo systemctl daemon-reload
sudo systemctl start commissioning-server.service
status=$(sudo service commissioning-server status)
#echo $status
sleep 1
#status=$(./clientNew 127.0.0.1 5433 "0000000020[{\"username\": \"rushikesh\",\"passcode\":\"finecho@178\",\"authkey\":\"abcd\",\"endpoint\":\"showUTCtimeAndLocaltime\"}]")
status=$(./clientNew 127.0.0.1 5433 0000000020"[{\"username\": \"rushikesh\",\"passcode\":\"finecho@178\",\"authkey\":\"abcd\",\"endpoint\":\"PlcDataloginStatus\",\"UTCTime\":\"2022-06-27 06:28:40\"}]")
#echo $status
echo $status > result.json
read contains < <(jq .field[0].result result.json)
#echo $contains

if [ "$contains" == "\"success\"" ]; then
  echo "Data logging as expected"
else 
	echo "Data not logging as expected"
fi