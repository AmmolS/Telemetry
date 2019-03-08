﻿#define BOARD_NUM 0

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "SerialPort.h"
#include "cbw.h"

#include <conio.h>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <cstdio>
#include <chrono>
#include <thread>
#include <windows.h>
#include <time.h>
#include <vector>

#include <jsoncpp.cpp> // include from dist in project folder
#include <json/json.h>
#include <json/json-forwards.h>

using namespace std;

// Arduino portname
const char *port_name = "\\\\.\\COM4";

//String for incoming data from Arduino
char incomingData[MAX_DATA_LENGTH];

void update_filename(vector<string> fileNames) {
	// Create new JSON file (or truncate existing contents)
	std::ofstream jsFile("JSFileName/FileList.json", std::ofstream::trunc);

	Json::Value data;
	Json::Value name(Json::arrayValue);

	// Add file names to JSON file
	for (size_t i = 0; i < fileNames.size(); i++) {
		name.append(fileNames.at(i));
	}
	
	data["File Name"] = name;

	jsFile << data << std::flush;
	jsFile.close();
}


void write_to_JSON(vector<string> time, vector<double> T1, vector<double> T2, vector<double> pressure) {

	// Initializing overarching JSON object
	Json::Value data;

	// Initializing JSON vectors
	Json::Value tc1(Json::arrayValue);
	Json::Value tc2(Json::arrayValue);
	Json::Value tc3(Json::arrayValue);
	Json::Value tc4(Json::arrayValue);

	Json::Value pt1(Json::arrayValue);
	Json::Value pt2(Json::arrayValue);
	Json::Value pt3(Json::arrayValue);

	// Appending temperature values to vectors
	tc1.append(Json::Value(T1.at(1)));
	tc1.append(Json::Value(T1.at(2)));
	tc1.append(Json::Value(T1.at(3)));
	tc1.append(Json::Value(T1.at(4)));
	tc1.append(Json::Value(T1.at(5)));
	tc1.append(Json::Value(T1.at(6)));
	
	tc2.append(Json::Value(T2.at(1)));
	tc2.append(Json::Value(T2.at(2)));
	tc2.append(Json::Value(T2.at(3)));
	tc2.append(Json::Value(T2.at(4)));
	tc2.append(Json::Value(T2.at(5)));
	tc2.append(Json::Value(T2.at(6)));
	
	tc3.append(Json::Value(48));
	tc3.append(Json::Value(49));
	tc3.append(Json::Value(50));
	tc3.append(Json::Value(50));
	tc3.append(Json::Value(49));
	tc3.append(Json::Value(49));

	tc4.append(Json::Value(60));
	tc4.append(Json::Value(60));
	tc4.append(Json::Value(61));
	tc4.append(Json::Value(60));
	tc4.append(Json::Value(64));
	tc4.append(Json::Value(64));

	pt1.append(Json::Value(pressure.at(0)));
	pt1.append(Json::Value(pressure.at(1)));
	pt1.append(Json::Value(pressure.at(2)));
	pt1.append(Json::Value(pressure.at(3)));
	pt1.append(Json::Value(pressure.at(4)));
	pt1.append(Json::Value(pressure.at(5)));

	pt2.append(Json::Value(pressure.at(6)));
	pt2.append(Json::Value(pressure.at(7)));
	pt2.append(Json::Value(pressure.at(8)));
	pt2.append(Json::Value(pressure.at(9)));
	pt2.append(Json::Value(pressure.at(10)));
	pt2.append(Json::Value(pressure.at(11)));

	pt3.append(Json::Value(pressure.at(12)));
	pt3.append(Json::Value(pressure.at(13)));
	pt3.append(Json::Value(pressure.at(14)));
	pt3.append(Json::Value(pressure.at(15)));
	pt3.append(Json::Value(pressure.at(16)));
	pt3.append(Json::Value(pressure.at(17)));

	// JSON will have two branches underneath data, one for the DAQ, and one for the Arduino
	data["DAQ"]["PT1"] = pt1;
	data["DAQ"]["PT2"] = pt2;
	data["DAQ"]["PT3"] = pt3;
	data["DAQ"]["Load Cell"] = "test";
	data["Arduino"]["Thermocouple1"] = tc1;
	data["Arduino"]["Thermocouple2"] = tc2;
	data["Arduino"]["Thermocouple3"] = tc3;
	data["Arduino"]["Thermocouple4"] = tc4;
	data["Arduino"]["Load Cell"] = "test";

	std::ofstream ofs("Output/" + time.back(), std::ofstream::app);

	ofs << data << std::flush;
	ofs.close();
}

int main()
{
	// MCC board setup
	int BoardNum = 0;
	int Gain = BIP10VOLTS;
	unsigned short Data = 0;
	float EngUnits = 0;
	double pressureKP = 0;
	short BoardStatus;

	// MCC sampling
	long Count = 18; // Total number of samples to be collected (6 samples * 3 channels)
	long Rate = 5000; // Rate that samples are collected per channel
	HANDLE MemHandle = cbWinBufAlloc(Count);
	WORD *ADData = (WORD*)MemHandle;

	vector<double> T1;
	vector<double> T2;
	vector<double> pressure;
	vector<string> fileNames;

	SerialPort arduino(port_name);

	if (!arduino.isConnected()) {
		cout << "Arduino is not connected." << endl;
	}
	else {
		cout << "Connection established with Arduino." << endl;
		Sleep(3000); // Give time to initialize Arduino
	}

	// Scan channels 0 to 2 on DAQ
	BoardStatus = cbAInScan(BoardNum, 0, 2, Count, &Rate, Gain, MemHandle, CONVERTDATA);
	
	if (BoardStatus == 343) {
		cout << "MCC DAQ is not connected. Code: " << BoardStatus << endl;
	}
	else if (BoardStatus != 0) {
		cout << "Error with MCC DAQ. Code: " << BoardStatus << endl;
	}
	else {
		cout << "Connection established with MCC DAQ." << endl;
	}

	clock_t time = clock();
	while (!_kbhit() && (arduino.isConnected() || BoardStatus == 0)) {

		if (arduino.isConnected()) {
			//Check if data has been read or not
			int read_result = arduino.readSerialPort(incomingData, MAX_DATA_LENGTH);

			istringstream raw(incomingData);
			string thermocouple;
		
			// Temperature value of -999 is an error/invalid value
			while (getline(raw, thermocouple, '-')) {
				// Find all valid numbers in received string and push them to temperature vector
				if (thermocouple.find("T1") != string::npos && thermocouple.length() > thermocouple.find("T1") + 7)
				{
					T1.push_back(stod(thermocouple.substr(thermocouple.find("T1") + 3, 5)));
				}
				else
				{
					T1.push_back(-999);
				}


				if (thermocouple.find("T2") != string::npos && thermocouple.length() > thermocouple.find("T2") + 7)
				{
					T2.push_back(stod(thermocouple.substr(thermocouple.find("T2") + 3, 5)));
				}
				else
				{
					T2.push_back(-999);
				}

			}

		}

		// Force vector to have size of 6
		while (T1.size() <= 6) {
			T1.push_back(-999);
		}

		while (T2.size() <= 6) {
			T2.push_back(-999);
		}
		
		if (BoardStatus == 0) {
			printf("MCC: \n");
			for (int i = 0; (i < 18); i++)
			{
				cbToEngUnits(BoardNum, Gain, ADData[i], &EngUnits);
				//pressureKP = 250 * EngUnits - 250; // +1V to +5V range
				pressureKP = 50 * EngUnits + 500; // -10V to +10V range
				printf("%4u, %fV, %lfP\n", ADData[i], EngUnits, pressureKP);
				pressure.push_back(pressureKP);
			}
		}
		else {
			for (int i = 0; i < 18; i++) {
				pressure.push_back(-999);
			}
		}

		if (arduino.isConnected()) {
			printf("\nArduino: \n");
			printf("%s", incomingData);
		}

		fileNames.push_back(to_string(clock() - time) + "ms.json");
		write_to_JSON(fileNames, T1, T2, pressure);
		update_filename(fileNames);

		pressure.clear();
		T1.clear();
		T2.clear();

		// Wait 0.5 seconds (time between each file write operation)
		Sleep(500);
	}
	// Remove files in Output folder
	for (size_t i = 0; i < fileNames.size(); ++i) {
		remove(("Output/" + fileNames.at(i)).c_str());
	}
	
	free(ADData); 
	return 0;
}
