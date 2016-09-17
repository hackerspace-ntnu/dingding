# -*- coding: utf-8 -*-
from __future__ import print_function
import datetime
from pymongo import MongoClient
    
class Slack_notify:
    weekdays2=("mandager","tirsdager","onsdager","torsdager","fredager")        
    def __init__(self):
        self.client = MongoClient()
        self.db= self.client.test
        self.users= self.db.users
        self.timestamp = self.db.timestamp
        
    """
        Legger til bruker i databasen/oppdaterer tidspunk om bruker allerede er registrert.
        
        Args:
            user (string): brukernavn på slack
            day (int): ukedag brukeren skal få notifications. 0-4 for mandag-fredag
            ts (int): Timeslot for notifications. 0: 10-12, 1: 12-14 ... 3: 16-18
            
    
    """        
    def add_user(self,user,day,ts):
        if day<0 or day>4 or ts<0 or ts>3:
            return False
            
        db_user = self.users.find_one({"user":user})
        if db_user is not None:
            db_user["day"] = day
            db_user["ts"] = ts
            self.users.save(db_user)
        else:
            self.users.insert_one({"user":user,"day":day,"ts":ts})
        
        return True

    def remove_user(self,userid):
        user = self.users.find_one({"user":userid})
        if user is not None:
            self.users.remove(user)
            
    """
        Returnerer ei liste over alle brukerere som er registrert på tidspunket nå.
        Timeslot 0: 10-12, 1: 12-14 osv 
        
    """
    def get_users_to_notify(self,day,timeslot):
        res = self.users.find({"day":day,"ts":timeslot})

        return [e["user"] for e in res]


    def get_message_history_start(self):
        res=self.timestamp.find_one()
        if res is None:
            self.timestamp.insert_one({"ts":0.0})
            return 0
        return res["ts"]

    def update_message_history_start(self,timestamp):
        res=self.timestamp.find_one()
        if res is not None:
            res["ts"]=timestamp
            self.timestamp.save(res)
        else:
            self.timestamp.insert_one({"ts":timestamp})

        
    """
        For debugging. Tømmer databasen!
    """
    def clear_db(self):
        for p in self.users.find():
            self.users.remove(p)
        for p in self.timestamp.find():
            self.timestamp.remove(p)
        
        
    
if __name__ == "__main__":
    slack = Slack_notify()
    print("Users:")
    for day in range(5):
        for ts in range(4):
            print("\n"+str(slack.weekdays2[day]) +" "+ str(ts*2+10) +"-"+str(ts*2+12) )
            for usr in slack.get_users_to_notify(day,ts):
                print(usr)
