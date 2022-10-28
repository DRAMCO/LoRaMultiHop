# This is a sample Python script.
import os

import serial
import json
import io
from datetime import datetime
import argparse
import csv
import ftplib
import configparser

import analyzer
import config
import payloadParser
import logFile

config = configparser.ConfigParser()
config.read('secret_config.ini')






def openFile(file_name):
    if len(file_name) == 0:
        f = []
        for (_, _, filenames) in os.walk(os.getcwd()):
            f.extend(filenames)
            break

        for f_n in f:
            if not (f_n.endswith('.json') or f_n.endswith('.csv')):
                f.remove(f_n)

        file_name = f[-1]

    print('Analysing ' + file_name)

    if file_name.endswith('.csv'):
        file_name = logFile.processCsv(file_name)

    return file_name



# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    valid_option = False

    parser = argparse.ArgumentParser("LoraMultiHop Logger and Parser")
    parser.add_argument("command", help="Should we analyze or log?")
    parser.add_argument("-p", "--port", dest="port", default=config["Serial"]["DefaultPort"], help="Serial port")
    parser.add_argument("-f", "--file", dest="file", default="",  help="Filename")
    parser.add_argument("-o", "--online", dest="online", help="Get file from online FTP source", nargs='?', const='')
    parser.add_argument("-s", "--signal", dest="signal", help="Signal Quality Report", nargs='?', const='')

    args = parser.parse_args()

    if args.command.lower() == "analyze" or args.command.lower() == "a":
        file_name = args.file
        if args.online is not None:
            file_name = ftpDownload(args.file)
        print("Running analysis...")
        analyzer.runAnalysis(file_name)
        if args.signal is not None:
            analyzer.runSignalAnalysis(file_name)


    elif args.command.lower() == "log" or args.command.lower() == "l":
        print("Starting logger...")
        print("Logging on (" + config["Serial"]["DefaultPort"] + "), change by specifying -p")
        logFile.runLogger(args.port)

    print('Program end')
