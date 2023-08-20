for ((i=1; i < 50; i++))
do
curl --header "Content-Type: application/json" --request POST --data '{"userName":"Player '$i'","mapId":"map1"}' http://localhost:8080/api/v1/game/join
echo "$i"
done

curl --header "Content-Type: application/json" --request POST --data '{"timeDelta":15000}' http://localhost:8080/api/v1/game/tick
echo
curl  --request GET 'http://localhost:8080/api/v1/game/records'
echo
echo Recods 20 10
curl  --request GET 'http://localhost:8080/api/v1/game/records?start=20&maxItems=10'
echo

