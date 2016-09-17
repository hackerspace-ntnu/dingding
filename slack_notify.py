# -*- coding: utf-8 -*-
import datetime
#from pymongo import MongoClient
import sqlite3 as lite    
class Slack_notify:
        
    def __init__(self):
        #self.client = MongoClient()
        #self.db= self.client.test
        #self.users= self.db.users
        #self.timestamp = self.db.timestamp
        self.con=lite.connect('/home/pi/App/database/slackdb.db')
    """
        Legger til bruker i databasen/oppdaterer tidspunk om bruker allerede er registrert.
        
        Args:
            user (string): brukernavn pao slack
            day (int): ukedag brukeren skal fo notifications. 0-4 for mandag-fredag
            ts (int): Timeslot for notifications. 0: 10-12, 1: 12-14 ... 3: 16-18
            
    
    """        
    def add_user(self,user,day,ts):
        if day<0 or day>4 or ts<0 or ts>3:
            return False
        sql2=u"INSERT OR REPLACE INTO users (uname,day,time) VALUES(?,?,?);"
        with self.con:
            cur = self.con.cursor()
            cur.execute(sql2,(user,str(day),str(ts)))
        
        return True

    def remove_user(self,userid):
        with self.con:
            cur = self.con.cursor()
            cur.execute("DELETE FROM users WHERE uname=?;",(userid))
            
    """
        Returnerer ei liste over alle brukerere som er registrert pÃ¥ tidspunket nÃ¥.
        Timeslot 0: 10-12, 1: 12-14 osv 
        
    """
    def get_users_to_notify(self):
        day = datetime.date.today().weekday()
        timeslot = (datetime.datetime.now().hour-10)//2
        return self.get_users_to_notify_at(day,timeslot)

    def get_users_to_notify_at(self,day,timeslot):
        #res = self.users.find({"day":day,"ts":timeslot})
         with self.con:
             cur = self.con.cursor()
             cur.execute("SELECT uname FROM users WHERE day=? AND time=?;", (str(day), str(timeslot)) )
             rows = cur.fetchall()
             return [row[0] for row in rows]

    def get_all_users(self):
        with self.con:
            cur=self.con.cursor()
            cur.execute("SELECT uname FROM users")
            rows =cur.fetchall()
            return [row[0] for row in rows]    

    def get_message_history_start(self):
        with self.con:
             cur = self.con.cursor()
             cur.execute("SELECT tid FROM time;")
             return cur.fetchone()[0]

    def update_message_history_start(self,timestamp):
        with self.con:
             cur = self.con.cursor()
             cur.execute("UPDATE time SET tid=?;",(str(timestamp)))

        
    """
        For debugging. TÃ¸mmer databasen!
    """
    def clear_db(self):
        with self.con:
             cur = self.con.cursor()
             cur.execute("DELETE FROM users")
        
        
    
    
