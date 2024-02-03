import 'dart:async';
import 'dart:convert';
import 'dart:typed_data';

import 'package:flutter/cupertino.dart';
import 'package:flutter/material.dart';
import 'package:flutter_bluetooth_serial/flutter_bluetooth_serial.dart';
import 'package:flutter_joystick/flutter_joystick.dart';
import 'package:robo_app/ColorPickerDialog.dart';

class RemoteControlPage extends StatefulWidget {
  final BluetoothDevice server;

  const RemoteControlPage({super.key, required this.server});

  @override
  _RemoteControlPage createState() => new _RemoteControlPage();
}

class _Message {
  int whom;
  String text;

  _Message(this.whom, this.text);
}
 

class _RemoteControlPage extends State<RemoteControlPage> 
  with SingleTickerProviderStateMixin
{
  static final clientID = 0;
  BluetoothConnection? connection;

  List<_Message> messages = List<_Message>.empty(growable: true);
  String _messageBuffer = '';

  bool isConnecting = true;
  bool get isConnected => (connection?.isConnected ?? false);

  bool isDisconnecting = false;

  HSVColor _color = HSVColor.fromAHSV(1, 0, 1, 0.5);

  TabController? _controller;

  @override
  void initState() {
    super.initState();

    _controller = TabController(
      length: 2,
      vsync: this
    );

    _controller!.addListener(() { 
      if(_controller!.index == 0) {
        _sendMessage("SENDNUDES");
      }
      if(_controller!.index == 1) {
        _sendMessage("SAVEMEFROMDESPAIR");
      }
    });

    BluetoothConnection.toAddress(widget.server.address).then((_connection) {
      print('Connected to the device');
      connection = _connection;
      setState(() {
        isConnecting = false;
        isDisconnecting = false;
      });

      connection!.input!.listen(_onDataReceived).onDone(() {
        // Example: Detect which side closed the connection
        // There should be `isDisconnecting` flag to show are we are (locally)
        // in middle of disconnecting process, should be set before calling
        // `dispose`, `finish` or `close`, which all causes to disconnect.
        // If we except the disconnection, `onDone` should be fired as result.
        // If we didn't except this (no flag set), it means closing by remote.
        if (isDisconnecting) {
          print('Disconnecting locally!');
        } else {
          print('Disconnected remotely!');
        }
        if (this.mounted) {
          setState(() {});
        }
      });
    }).catchError((error) {
      print('Cannot connect, exception occured');
      print(error);
    });
  }

  @override
  void dispose() {
    // Avoid memory leak (`setState` after dispose) and disconnect
    if (isConnected) {
      isDisconnecting = true;
      connection?.dispose();
      connection = null;
    }

    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final serverName = widget.server.name ?? "Unknown";
    return DefaultTabController(
        length: 2,
        child: Scaffold(
            appBar: AppBar(
                //backgroundColor: Theme.of(context).colorScheme.inversePrimary,
                title: (isConnecting
               ? Text('waiting for ' + serverName + ' to beep beep')
               : isConnected
                   ? Text('beep beeping ' + serverName)
                   : Text('no longer beep bepping ' + serverName)),
                bottom: TabBar(
                  controller: _controller!,
                  tabs: [
                    Tab(text: "Fernsteuerung"),
                    Tab(text: "Linefollower"), 
                ]
                )),
            body: TabBarView(
              controller: _controller,
              children: [
                SafeArea(
                  child: Stack(children: <Widget>[
                    Align(
                      alignment: const Alignment(0, 0.8),
                      child: Joystick(listener: (e) {
                        //_sendMoveMessage(0, -1);
                        _sendMoveMessage(e.x, e.y);
                      }),
                    ),
                    Column(
                      children: [
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            ElevatedButton(onPressed: (){
                              _sendMessage("RESET,BRO");
                            }, child: const Text("Reset"))
                          ],
                        )
                      ],
                    )
                  ],)
                ),
                
                const Text("Test")
              ],
            )));

    // Scaffold(
    //   appBar: AppBar(
    //       backgroundColor: Theme.of(context).colorScheme.inversePrimary,
    //       title: (isConnecting
    //           ? Text('Connecting to ' + serverName + '...')
    //           : isConnected
    //               ? Text('Remote Control for ' + serverName)
    //               : Text('Disconnected from ' + serverName))),
    //   body: DefaultTabController(
    //     length: 1,
    //     child: Scaffold
    //   )

    //   SafeArea(
    //     child: Stack(
    //       children: <Widget>[
    //         Column(
    //           children: <Widget>[
    //             Padding(
    //               padding: const EdgeInsets.symmetric(vertical: 20),
    //               child: Row(
    //                 mainAxisAlignment: MainAxisAlignment.center,
    //                 children: <Widget>[
    //                   ElevatedButton(
    //                     onPressed: (){
    //                       Navigator.of(context).push(
    //                         CupertinoModalPopupRoute(
    //                           builder: (context) => ColorPickerDialog(
    //                             onColorChange: (color) {
    //                               var newColor = color.toColor();
    //                               _sendMessage("C${ newColor.value.toRadixString(16).padLeft(8, '0') }");
    //                             },
    //                           )
    //                         )
    //                       );
    //                     },
    //                     child: const Text("Test!")
    //                   ),
    //                 ]
    //               ),
    //             ),
    //           ],
    //         ),
    //         Align(
    //           alignment: const Alignment(0, 0.8),
    //           child: Joystick(listener: (e){
    //             _sendMoveMessage(e.x, e.y);
    //           }),
    //         ),
    //       ],
    //     ),
    //   ),
    // );
  }

  void _onDataReceived(Uint8List data) {
    // Allocate buffer for parsed data
    int backspacesCounter = 0;
    data.forEach((byte) {
      if (byte == 8 || byte == 127) {
        backspacesCounter++;
      }
    });
    Uint8List buffer = Uint8List(data.length - backspacesCounter);
    int bufferIndex = buffer.length;

    // Apply backspace control character
    backspacesCounter = 0;
    for (int i = data.length - 1; i >= 0; i--) {
      if (data[i] == 8 || data[i] == 127) {
        backspacesCounter++;
      } else {
        if (backspacesCounter > 0) {
          backspacesCounter--;
        } else {
          buffer[--bufferIndex] = data[i];
        }
      }
    }

    // Create message if there is new line character
    String dataString = String.fromCharCodes(buffer);
    int index = buffer.indexOf(13);
    if (~index != 0) {
      setState(() {
        messages.add(
          _Message(
            1,
            backspacesCounter > 0
                ? _messageBuffer.substring(
                    0, _messageBuffer.length - backspacesCounter)
                : _messageBuffer + dataString.substring(0, index),
          ),
        );
        _messageBuffer = dataString.substring(index);
      });
    } else {
      _messageBuffer = (backspacesCounter > 0
          ? _messageBuffer.substring(
              0, _messageBuffer.length - backspacesCounter)
          : _messageBuffer + dataString);
    }
  }

  void _sendMoveMessage(double x, double y) async {
    String encodeCoordinate(double val) {
      return ((-val + 1.0) * 127).floor().toRadixString(16).padLeft(2, '0');
    }

    _sendMessage("M${encodeCoordinate(x)}${encodeCoordinate(y)}");
  }

  void _sendMessage(String text) async {
    text = text.trim();
    if (text.length > 0) {
      try {
        print(text);
        connection!.output.add(Uint8List.fromList(utf8.encode(text + "\n")));
        await connection!.output.allSent;
      } catch (e) {
        // Ignore error, but notify state
        setState(() {});
      }
    }
  }
}
