# Logger and analyzer for LoRaMultiHop
To run code, make a `config.ini` file in the same directory as main.py: 
```
[FTP]
Server = ftp.dramco.be
User = dramco.be
Password = ******
Directory = /tools/lora-multihop/logs

[Serial]
DefaultPort = COM31
```
Run code using the following commands: 
```
usage: LoraMultiHop Logger and Parser [-h] [-p PORT] [-f FILE] [-o [ONLINE]] command

positional arguments:
  command               Should we analyze (analyze/a) or log (log/l)?

options:
  -h, --help            show this help message and exit
  -p PORT, --port PORT  Serial port
  -f FILE, --file FILE  Filename
  -o [ONLINE], --online [ONLINE]
                        Get file from online FTP source
```