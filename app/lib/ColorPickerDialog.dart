import 'package:flutter/material.dart';
import 'package:flutter_hsvcolor_picker/flutter_hsvcolor_picker.dart';

class ColorPickerDialog extends StatefulWidget {
  ColorPickerDialog({super.key, this.onColorChange});

  @override
  _ColorPickerDialog createState() => _ColorPickerDialog();

  void Function(HSVColor)? onColorChange;
}

class _ColorPickerDialog extends State<ColorPickerDialog> {
  HSVColor _color = HSVColor.fromColor(Colors.black);

  @override
  Widget build(BuildContext context){
    return AlertDialog(
      title: Text("Test Dialog"),
      content: SizedBox(
        width: 260,
        child: WheelPicker(
          color: _color, 
          onChanged: (color){
            if(widget.onColorChange != null){
              widget.onColorChange!(color);
            }
            setState(() {
              _color = color;
            });
          }
        ), 
      ),
      actions: <Widget>[
        Center(
          child: TextButton(
            onPressed: (){
              Navigator.of(context).pop();
            }, 
            child: Text("OK")
          ),
        )
      ],
    );
  }
  
}