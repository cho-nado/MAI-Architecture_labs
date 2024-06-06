set -e

mongo <<EOF
db = db.getSiblingDB('deliverydb')
db.createCollection('delivery')
db.delivery.createIndex({delivery_id: 1}, {unique: true})
db.delivery.createIndex({sender_id: 1})
db.delivery.createIndex({receiver_id: 1})
EOF