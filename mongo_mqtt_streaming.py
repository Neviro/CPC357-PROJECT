import pymongo
import paho.mqtt.client as mqtt
import json
from datetime import datetime, timezone
from pymongo.mongo_client import MongoClient
from pymongo.server_api import ServerApi

uri = "mongodb+srv://dlau:gvptZb3ULYsCxZO9@cluster-357.2bqlu.mongodb.net/?retryWrites=true&w=majority&appName=Cluster-357"
# Create a new client and connect to the server
mongo_client = pymongo.MongoClient(uri, server_api=ServerApi('1'))

# Send a ping to confirm a successful connection
try:
    mongo_client.admin.command('ping')
    print("Pinged your deployment. You successfully connected to MongoDB!")
except Exception as e:
    print(e)

# MongoDB configuration
db = mongo_client["smart_carpark"]
collection = db["realtime_data"]

# MQTT configuration
mqtt_broker_address = "" #### !!!! might need to update again whenever gcp vm restarted
mqtt_topic = "iot"

# Define the callback function for connection
def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"Successfully connected")
        client.subscribe(mqtt_topic)

# Define the callback function for ingesting data into MongoDB
def on_message(client, userdata, message):
    payload = message.payload.decode("utf-8")
    print(f"Received message: {payload}")
    
    # Convert MQTT timestamp to datetime
    timestamp = datetime.now(timezone.utc)
    datetime_obj = timestamp.strftime("%Y-%m-%dT%H:%M:%S.%fZ")
    
    data = json.loads(payload)

    # Assign values to variables
    TotalCar = data["TotalCar"]
    EvCar = data["EvCar"]
    PetrolCar = data["PetrolCar"]
    AirQuality = data["AirQuality"]
    Parking1Full = data["Parking1Full"]
    Parking2Full = data["Parking2Full"]

    # Process the payload and insert into MongoDB with proper timestamp
    document = {"timestamp": datetime_obj, "TotalCar": TotalCar, "EvCar": EvCar, "PetrolCar": PetrolCar, "AirQuality": AirQuality, "Parking1Full": Parking1Full, "Parking2Full": Parking2Full}
    collection.insert_one(document)
    print("Data ingested into MongoDB")

# Create a MQTT client instance
client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)

# Attach the callbacks using explicit methods
client.on_connect = on_connect
client.on_message = on_message

# Connect to MQTT broker
client.connect(mqtt_broker_address, 1883, 60)

# Start the MQTT loop
client.loop_forever()
