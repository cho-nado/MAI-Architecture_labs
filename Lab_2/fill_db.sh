for i in {1..20}
do
    curl -d "login=shelbiq$i&password=bober$i&email=bober$i@mail.ru&name=Thomas&surname=Shelby" -X POST localhost:8080/user
done
