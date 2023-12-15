


import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';

class SelectDevicePage extends StatefulWidget {
  SelectDevicePage({super.key});

  @override
  State<SelectDevicePage> createState(){
    return _SelectDevicePage();
  }
}

class _SelectDevicePage extends State<SelectDevicePage> {
  var bondedDevices = List<BluetoothDevice>.empty(growable: true);
  var otherDevices = List<BluetoothDevice>.empty(growable: true);
  bool _isDiscovering = false;
  bool _isBonding = false;
  StreamSubscription<BluetoothDiscoveryResult>? _streamSubscription;

  void _startDiscovery(){
    setState(() {
      bondedDevices.clear();
      otherDevices.clear();
      _isDiscovering = true;
    });
    _streamSubscription = FlutterBluetoothSerial.instance.startDiscovery().listen((r){
      print("Found Device ${r.device.address}");
      setState(() {
        if(r.device.bondState.isBonded){
          final index = bondedDevices.indexWhere((element) => element.address == r.device.address);
          if(index >= 0) {
            bondedDevices[index] = r.device;
          }
          else {
            bondedDevices.add(r.device);
          }
        }
        else {
          final index = otherDevices.indexWhere((element) => element.address == r.device.address);
          if(index >= 0) {
            otherDevices[index] = r.device;
          }
          else {
            otherDevices.add(r.device);
          }
        }
      });
    });
    _streamSubscription?.onDone(() {
      setState(() {
        _isDiscovering = false;
      });
    });
  }

  void _stopDiscovery(){
    setState(() {
      _streamSubscription?.cancel();
      _isDiscovering = false;
    });
  }

  Future<bool?> _bondDevice(BluetoothDevice device) {
    return FlutterBluetoothSerial.instance.bondDeviceAtAddress(device.address).timeout(Duration(seconds: 30));
  }
  
  @override
  void initState() {
    super.initState();
    _startDiscovery();
  }

  @override
  void dispose(){
    _streamSubscription?.cancel();
    super.dispose();
  }

  @override
  Widget build(context) {
    Widget makeDeviceTileEntry(device, onTap){
      return ListTile(
        title: Text(device.name == null? device.address : "${device.name} [${device.address}]"),
        onTap: onTap,
      );
    }
    List<Widget> bondedEntries = bondedDevices.map((e) => makeDeviceTileEntry(e, (){
      Navigator.of(context).pop(e);
    })).toList();
    List<Widget> unbondedEntries = otherDevices.map((e) => makeDeviceTileEntry(e, () async {
      if(_isBonding) return;
      _isBonding = true;
      try{
        final paired = (await _bondDevice(e))!;
          if(paired) {
            print("Pairing successful!");
          }
          else {
            print("Pairing failed!");
          }
      }
      catch(e) {

      }
    })).toList();

    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        actions: <Widget>[
          _isDiscovering ? 
            FittedBox(
                  child: Container(
                    margin: new EdgeInsets.all(16.0),
                    child: CircularProgressIndicator(
                      valueColor: AlwaysStoppedAnimation<Color>(
                        Theme.of(context).colorScheme.primary,
                      ),
                    ),
                  ),
                ) :
          IconButton(
            icon: Icon(Icons.replay),
            onPressed: (){
              _startDiscovery();
            },
          )
        ]
      ),
      body: ListView(
        children: <Widget>[
          ListTile(
            title: Text("Paired Devices:"),
          ),
          Expanded(
            child: ListView(
                shrinkWrap: true,
                physics: ClampingScrollPhysics(),
              children: bondedEntries,
            )
          ),
          Divider(),
          ListTile(
            title: Text("Unpaired Devices"),
          ),
          Expanded(
            child: ListView(
                shrinkWrap: true,
                physics: ClampingScrollPhysics(),
              children: unbondedEntries
            )
          )
        ]
      ),
    );
  }
}