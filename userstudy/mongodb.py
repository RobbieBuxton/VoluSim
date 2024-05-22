from pymongo import MongoClient

def connect_to_mongo():
    """ Connects to MongoDB and returns the database object. """
    # Creation of MongoClient
    client = MongoClient("mongodb://localhost:27017/", serverSelectionTimeoutMS=5000)
    try:
        client.server_info()  # Force connection on a request as the driver doesn't connect until you attempt to send some data
    except Exception as e:
        print("Unable to connect to the server:", e)
        return None
    # Access database
    mydatabase = client["user_study_db"]

    return mydatabase