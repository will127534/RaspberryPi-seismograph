import numpy as np
from datetime import datetime,timedelta
import dateutil.parser
import scipy.io as sio
import os
from obspy.core import Trace,Stream,UTCDateTime
import requests
import lxml
from lxml import etree


process_date = datetime.now()- timedelta(days=1)
process_date = process_date.strftime("%Y_%m_%d")
File_date = datetime.now()- timedelta(days=1)
File_date = File_date.strftime("%Y%m%d")
print("Processing:%s",process_date)

#Download the data from Google Drive
os.system('rclone copy google:earthquake/%s /root/earthquaketemp -vv -P --retries 1 --no-traverse --fast-list --transfers 100  --include=%s_*.txt' % (process_date,File_date))


#Fetching Earthquake Event from CWB webpage
r = requests.get('https://www.cwb.gov.tw/V7/modules/MOD_EC_Home.htm')
html = etree.HTML(r.text)
Time_result = html.xpath('//div[@class="earthshockinfo02"]/table/tbody/tr/td[2]')
Mag_result = html.xpath('//div[@class="earthshockinfo02"]/table/tbody/tr/td[5]')

events = []
for x in range(len(Time_result)):
    singleevent = dict()
    eventtime = datetime.strptime(Time_result[x].text + ":00.00","%m/%d %H:%M:%S.%f")
    eventtime = eventtime.replace(year = datetime.now().year)
    eventtime = UTCDateTime(eventtime)
    singleevent['time'] = eventtime
    singleevent['text'] = "M" + Mag_result[x].text
    events.append(singleevent)
    print("%s,%s" % (eventtime,"M" + Mag_result[x].text))
    pass

#Reading the data
datatype = [('Time', '<f8'), ('X', '<f8'), ('Y', '<f8'), ('Z', '<f8')]
data = []
for filename in sorted(os.listdir("/root/earthquaketemp")):
	if filename.endswith(".txt"):
		print(filename)
		filename_date = filename
		data2 = np.genfromtxt('/root/earthquaketemp/' + filename,dtype=datatype, delimiter=',',invalid_raise=False)
		try:
			data = np.concatenate((data,data2))
			pass
		except Exception as e:
			data = data2

#Saving the concatenated file to disk
np.save(filename_date[0:8],data)

starttime = UTCDateTime(data['Time'][0]) +timedelta(hours=8)
length = len(data['Time'])
sampling_rate =length/ (data['Time'][-1] - data['Time'][0])

#graph size
size = (210*5,210*5)

#Processing each channel to prevent memory error reguarding matplotilb
for x in ['X','Y','Z']:
	statsx= {'network': 'TW', 
	'station': 'RASPI', 
	'location': '00', 
	'channel': 'BH'+x, 
	'npts': length, 
	'sampling_rate': sampling_rate, 
	'mseed' : {'dataquality' : 'D'}, 
	'starttime': starttime}
	Xt = Trace(data=data[x], header=statsx)
	Xt_filt = Xt.copy()
	Xt_filt.filter('lowpass', freq=20.0, corners=2, zerophase=True)
	stream = Stream(traces=[Xt_filt])
	stream.plot(type='dayplot',outfile='dayplotFilter'+x+'.png',size=size,events=events)
	stream = Stream(traces=[Xt])
	stream.plot(type='dayplot',outfile='dayplot'+x+'.png',size=size,events=events)

#Remove all the download and generated files
os.system('rm -rf /root/earthquaketemp')
os.system('rclone copy /root  google:earthquake/%s -vv -P --retries 1 --no-traverse --fast-list --transfers 10 --include=*.png  --include=*.npy' % (process_date))
os.system('rm -rf /root/*.png')
os.system('rm -rf /root/*.npy')