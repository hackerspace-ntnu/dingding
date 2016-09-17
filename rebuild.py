# -*- coding: utf-8 -*-
from __future__ import print_function
from slackclient import SlackClient
from slack_notify import Slack_notify

import sys
import time
import datetime
import threading
import traceback
import pymongo
import random

class Slackman():

    #token = "xoxp-3579817643-11704551651-33308548417-a9bfab185b"
    token = "xoxb-33668701680-OcoqOBhNZ5I09XlCfME6Tq6g"
    channel_key ="C0Z901HA5"
    weekdays={"mon":0,"tue":1,"wed":2,"thu":3,"fri":4, "man":0,"tir":1,"ons":2,"tor":3,"fre":4}
    weekdays2=("mandager","tirsdager","onsdager","torsdager","fredager")
    to_send=False
    warning_flag=False
    back_to_life_flag = False
    errorMsg = None
    alive =True
    last_message_check=0
    registered=set()
    sc = SlackClient(token)
    sn = Slack_notify()
    

    def run(self):
        reload(sys)
        sys.setdefaultencoding("utf-8")
        #self.sn.update_message_history_start(1460415173)
        while self.alive:
            
            if (time.time() - self.last_message_check) >10:
                tid= self.scan_messages()
                self.sn.update_message_history_start(tid)
                self.last_message_check=time.time()
            time.sleep(1)
    
    def scan_messages(self,latest_time=time.time()):

        ret = self.sc.api_call("channels.history",channel=self.channel_key,latest=latest_time)

        last_msg_time=0
        if ret is not None and "messages" in ret:
            for a in ret["messages"]:
                last_msg_time=float(a["ts"])
                if "user" in a:
                    userid= a["user"]
                    if userid in self.registered:
                        continue

                    text=a["text"].encode('utf-8')        
                    data = text.lower().strip()
                    data=data.split(" ")
                    if len(data)<2 or data[0].replace(':','')!="<@u0zknlml0>":
                        continue

                    
                    if(len(data)<3):
                        continue


                    try:
                        weekday=self.weekdays[data[1][0:3]]
                        timeslot = (int(data[2])-10)//2
                        if timeslot<0 or timeslot>4:
                            raise ValueError("Invalid time")
                    except:
                        continue
                    self.registered.add(userid)
                    print(userid)
                    self.sn.add_user(userid,weekday,timeslot)

            if ret["has_more"]:
                return self.scan_messages(last_msg_time)
        

        return time.time()
        #return max(last_msg_time,start_time)


if __name__=="__main__":
    slackman = Slackman()
    slackman.run()
   # slackman.print_users()
