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
import os

class Slackman(threading.Thread):

    token = os.environ["API_KEY"]
    channel_key ="C0Z901HA5"
    weekdays={"mon":0,"tue":1,"wed":2,"thu":3,"fri":4, "man":0,"tir":1,"ons":2,"tor":3,"fre":4}
    weekdays2=("mandager","tirsdager","onsdager","torsdager","fredager")
    greetings = ("sup", "skjer", "skjera", "wazzap", "whatsapp", "sup?", "skjer?", "skjera?", "wazzap?", "whatsapp?")
    greetings_responses = ("ingenting, dingeling!", "ins, dd?")
    to_send=False
    warning_flag=False
    back_to_life_flag = False
    errorMsg = None
    alive =True
    last_message_check=0
    manding = False

    sc = SlackClient(token)
#    sn = Slack_notify()
    

    def __init__(self):
        print("Token: ",self.token)
        threading.Thread.__init__(self)

    def run(self):
        reload(sys)
        sys.setdefaultencoding("utf-8")
        #self.sn.update_message_history_start(1460415173)
        self.sn=Slack_notify()
        while self.alive:
            
            try:
                if self.to_send:
                    self.send_notification()
                    self.to_send=False

                if self.warning_flag:
                    self.warning_flag=False
                    tiden = datetime.datetime.now().hour
                    response=""
        #            if tiden>=7 and tiden <=22:
       #                 response = "Noe er galt med ringeklokken. I need a doctor! <@U0BLQG7K5> <@U04JGAXFF>"
      #              else:
     #                   response = "Noe er galt med ringeklokken. I need a doctor!"
    #                self.post_message(response)

   #             if self.back_to_life_flag:
  #                  self.back_to_life_flag = False
 #                   self.post_message(u"Ringeklokken har gjenoppstått!")
#
  #              if self.errorMsg:
 #                  self.post_message("Noe gikk veldig galt:\n" + self.errorMsg)
#                   self.errorMsg = None

                if (time.time() - self.last_message_check) >10:
                    tid= self.scan_messages(self.sn.get_message_history_start())
                    self.sn.update_message_history_start(tid)
                    self.last_message_check=time.time()
                time.sleep(1)
            except pymongo.errors.PyMongoError:
                self.post_message("Noe er galt med MongoDB: <@U0BLQG7K5> <@U04JGAXFF>\n" + traceback.format_exc())
                self.alive = False
            except KeyboardInterrupt:
                print("bye from slackman")
                self.alive = False
            except:
                print("fucked up")
                self.error(traceback.format_exc())
                time.sleep(5)    
    
    def notify(self):
        self.to_send=True

    def warning(self):
        self.warning_flag = True

    def error(self, err):
        self.errorMsg = err
        print(err)

    def back_to_life(self):
        self.back_to_life_flag = True

    def send_notification(self):
        vakter=self.sn.get_users_to_notify()
        message=u"Noen har ringt på hos Hackerspace!"
        for vakt in vakter:
            print(vakt)
            message+=" <@"+vakt+">"
        self.post_message(message)

    def is_manding(self):
        if self.manding:
            self.manding=False
            return True
        return False

    def print_users(self):
        for day in range(5):
            for ts in range(4):
                print("\n"+str(self.weekdays2[day]) +" "+ str(ts*2+10) +"-"+str(ts*2+12) )
                for usr in self.sn.get_users_to_notify_at(day,ts):
                    res=self.sc.api_call("users.info",user=usr)
                    if res["ok"]:
                        res=res["user"]
                    else:
                        continue
                    print(res["real_name"]+"  ("+res["name"]+", "+usr+")")

    def post_message(self,message):
        self.sc.api_call("chat.postMessage", channel="#ding-dong", text=message,username='Dingding', icon_emoji=':pekkaross:')

    def ugyldig_input(self,message,userid):

        response="<@"+userid+u">: Jeg forstår ikke \"" +message + "\"."
        #print(response)
        self.post_message(response)

    """
        For å melde seg av send "stopp" i chat
    """
    def stopp(self,userid):
        self.sn.remove_user(userid)
        response="<@"+userid+u">: Du får ikke lenger varslinger."
       # print(response)
        self.post_message(response)



    def scan_messages(self,start_time=0,latest_time=None,send_push_nots=True):

        if start_time==0.0:
            send_push_nots=False
        try:
            if latest_time is None:
                ret = self.sc.api_call("channels.history",channel=self.channel_key,oldest=(start_time+0.1))
            else:
                ret = self.sc.api_call("channels.history",channel=self.channel_key,oldest=(start_time+0.1),latest=latest_time)
        except:
            return start_time #don't care. plz dont spam stack trace
        last_msg_time=0
        if ret is not None and "messages" in ret:
            for a in reversed(ret["messages"]):
                print(a)
                last_msg_time=float(a["ts"])
                if "user" in a:
                    userid= a["user"]
                    text=a["text"].encode('utf-8')        
                    data = text.lower().strip()
                    data=data.split(" ")
                    if len(data)<2 or data[0].replace(':','')!="<@u0zknlml0>":
                        #print(data)
                        continue

                    if len(data)==2 and data[1]=="stopp":
                        self.stopp(userid)
                        continue
                    if len(data)>=2 and data[1] in self.greetings:
                        self.post_message("<@"+userid+u">: " +random.choice(self.greetings_responses) )
                        continue

                    if len(data)==2 and (data[1]=="ding" or data[1] == "dong" or data[1] == "dang"):
                        if "i" in data[1]:
                            #response = random.choice(["dong", "dang"])
                            dingtime = self.sn.can_ding(userid)
                            if(dingtime>=30):
                                self.manding = True
                            else:
                                self.post_message("<@"+userid+u">: " + "Du kan dinge om "+ str(30-dingtime)+ " minutter!")
                        else:
                            response = "ding"
                            self.post_message("<@"+userid+u">: " + response)
                        continue
                    
                    if(len(data)<3):
                        if send_push_nots:
                            self.ugyldig_input(text,userid)
                        continue


                    try:
                        weekday=self.weekdays[data[1][0:3]]
                        timeslot = (int(data[2])-10)//2
                        if timeslot<0 or timeslot>4:
                            raise ValueError("Invalid time")
                    except:
                        if send_push_nots:
                            self.ugyldig_input(text,userid)
                        continue

                    self.sn.add_user(userid,weekday,timeslot)
                    if send_push_nots:
                        response="<@"+userid+u">: Du får varslinger på " +self.weekdays2[weekday] +" " + str(timeslot*2+10) +"-"+str(timeslot*2+12)+"!"
                        self.post_message(response)
            #else:
            #    return time.time(
            #if ret["has_more"]:
                #return self.scan_messages(start_time,latest_time=latest_time)
        

        return time.time()
        #return max(last_msg_time,start_time)


if __name__=="__main__":
    slackman = Slackman()
    #slackman.start()
    slackman.sn=Slack_notify()
    slackman.print_users()
