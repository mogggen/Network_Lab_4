package com.company;

import javax.swing.*;
import java.awt.*;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.util.ArrayList;

public class Main extends JComponent {

    // Coordinate system: -100 - 100
    // Resolution: 505x505

    static void Log(String msg, int clientId)
    {
        System.out.println("[" + seqNo + "][" + clientId + "]" + msg + "(" + newPlMsgcount + ", " + newPlPosMsgcount + ", " + plLevMsgcount + ")");
    }

    public static class Client
    {
        int clientId;
        Coordinate position;
        ObjectDesc objectDesc; // used for color
        ObjectForm objectForm;
    }

    static ArrayList<Client> info = new ArrayList<>();


    static int newPlMsgcount = 0;
    static int newPlPosMsgcount = 0;
    static int plLevMsgcount = 0;
    static ServerSocket ss;
    static Socket s;
    static InputStream in;
    static OutputStream out;

    //Enums och constants
    public int MAXNAMELEN = 32;

    enum ObjectDesc {
        Human,
        NonHuman,
        Vehicle,
        StaticObject
    };

    enum ObjectForm {
        Cube,
        Sphere,
        Pyramid,
        Cone
    };

    public static class Coordinate {
        public int x;
        public int y;
    };

    enum MsgType {
        Join,           //Client joining game at server
        Leave,          //Client leaving game
        Change,         //Information to clients
        Event,          //Information from clients to server
        TextMessage     //Send text messages to one or all
    };
    //MESSAGE HEAD, Included first in all messages
    public static class MsgHead {
        public int length;     //Total length for whole message
        public int seqNo;      //Sequence number
        public int id;         //Client ID or 0;
        public MsgType type;   //Type of message
    };
    //JOIN MESSAGE (CLIENT->SERVER)
    public static class JoinMsg {
        public MsgHead head;
        public ObjectDesc desc;
        public ObjectForm form;
        //public char name[];   //null terminated!,or empty
    };
    //LEAVE MESSAGE (CLIENT->SERVER)
    public static class LeaveMsg {
        public MsgHead head;
    };
    //CHANGE MESSAGE (SERVER->CLIENT)
    enum ChangeType {
        NewPlayer,
        PlayerLeave,
        NewPlayerPosition
    };
    //Included first in all Change messages
    public static class ChangeMsg {
        public MsgHead head;
        public ChangeType type;
    };

    public static class NewPlayerMsg {
        public ChangeMsg msg;          //Change message header with new client id
        public ObjectDesc desc;
        public ObjectForm form;
        //public char name[];  //nullterminated!,or empty
    };
    public static class PlayerLeaveMsg {
        public ChangeMsg msg;          //Change message header with new client id
    };
    public static class NewPlayerPositionMsg {
        public ChangeMsg msg;          //Change message header
        public Coordinate pos;         //New object position
        public Coordinate dir;         //New object direction
    };
    //EVENT MESSAGE (CLIENT->SERVER)
    enum EventType { Move };
    //Included first in all Event messages
    public static class EventMsg {
        public MsgHead head;
        public EventType type;
    };
    //Variations of EventMsg
    public static class MoveEvent {
        public EventMsg event;
        public Coordinate pos;         //New object position
        public Coordinate dir;         //New object direction
    };
    //TEXT MESSAGE
    public static class TextMessageMsg {
        public MsgHead head;
        //public char text[];   //NULL-terminated array of chars.
    };

    static int seqNo = 0;
    static int id = 0;
    //static Coordinate pos = new Coordinate();
    static byte direction = -1;

    enum dir
    {
        up,
        down,
        left,
        right
    }

    public static Client getClient(int clientId)
    {
        for (Client c: info) {
            if (c.clientId == clientId){
                return c;
            }
        }
        return null;
    }


    public static class GUI
    {
        JFrame frame = new JFrame();
        PixelCanvas canvas = new PixelCanvas();
        public GUI()
        {
            canvas.setBounds(0, 0, 303, 303);
            frame.setSize(303, 303);
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
            frame.setTitle("GUI Server");
            frame.add(canvas);
            frame.pack();
            frame.setVisible(true);
            frame.addKeyListener(new KeyListener() {
                @Override
                public void keyTyped(KeyEvent e) {

                }

                @Override
                public void keyPressed(KeyEvent event) {
                    switch (event.getKeyCode()) {
                        case 27 -> System.exit(0);

                        case 87, 38 -> direction = (byte)dir.up.ordinal();
                        case 83, 40 -> direction = (byte)dir.down.ordinal();
                        case 65, 37 -> direction = (byte)dir.left.ordinal();
                        case 68, 39 -> direction = (byte)dir.right.ordinal();
                    }
                    //final Runnable runnable = (Runnable) Toolkit.getDefaultToolkit().getDesktopProperty("win.sound.exclamation"); if (runnable != null) runnable.run();
                    try {
                        speak();
                    } catch (IOException exception) {
                        exception.printStackTrace();
                    }
                }

                @Override
                public void keyReleased(KeyEvent e) {

                }
            });
        }
    }

    static int Fit(int input, int inputStart, int inputEnd)
    {
        return (int)(((float) 255) / (inputEnd - inputStart) * (input - inputStart));
    }


    //handles the drawing of the input value
    static class PixelCanvas extends JComponent
    {
        ArrayList<Byte> xCoordBuf = new ArrayList<>();
        ArrayList<Byte> yCoordBuf = new ArrayList<>();
        ArrayList<Byte> colorIndexBuf = new ArrayList<>();
        int SetParamArr(ArrayList<Byte> xBuf,
                         ArrayList<Byte> yBuf,
                         ArrayList<Byte> color)
        {
            int deltaSize = this.xCoordBuf.size();
            this.xCoordBuf = xBuf;
            this.yCoordBuf = yBuf;
            this.colorIndexBuf = color;
            return 1; //xBuf.size() - deltaSize;
        }

        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g); // happens when the buffer.size changes
            System.out.println("Connections: " + info.size());
            if (info != null && info.size() > 0) {
                for (int i = 0; i < xCoordBuf.size(); i++){
                    if (colorIndexBuf.get(i) == 0){
                        return;
                    }
                    int up = Fit(colorIndexBuf.get(i), 1, 16);
                    int down = Fit(colorIndexBuf.get(i), 16, 1);
                    g.setColor(new Color(down, up, down));
                    g.fillRect(xCoordBuf.get(i), yCoordBuf.get(i), 10, 10);
                }
            }
        }

        @Override
        public Dimension getPreferredSize() {
            return new Dimension(505, 505);
        }
    }

    public static void runSetup(int port) throws IOException {
        ss = new ServerSocket(port);

        s = ss.accept();
        in = s.getInputStream();
        out = s.getOutputStream();

        // send joinMsg and receive

        byte[] buf = new byte[6]; // bytes in joinMsg

        //joinMsg.head.length
        buf[0] = 6;

        //joinMsg.head.seqNo
        buf[1] = (byte)++seqNo;

        //joinMsg.head.id
        buf[2] = (byte)id;

        //joinMsg.head.MsgType
        buf[3] = (byte)MsgType.Join.ordinal();

        //joinMsg.ObjectDesc
        buf[4] = (byte)ObjectDesc.Human.ordinal();

        //joinMsg.ObjectForm
        buf[5] = (byte) ObjectForm.Cone.ordinal();

        out.write(buf);
        System.out.println("forwarded joinMsg...");
    }


    //Reads Bytes from the input stream
    public static void listen(GUI window) throws IOException {

        byte[] buf = new byte[7];
        int count = in.read(buf);
        if (count == -1)
        {
            System.out.println("Error: unable to receive");
            System.exit(1);
        }

        //Log("[" + ChangeType.values()[buf[4]] + "]", buf[2]);
        if ((byte)ChangeType.NewPlayer.ordinal() == buf[4]){
            newPlMsgcount++;
            NewPlayerMsg newPlayerMsg = new NewPlayerMsg();
            newPlayerMsg.msg = new ChangeMsg();
            newPlayerMsg.msg.head = new MsgHead();

            newPlayerMsg.msg.head.length = buf[0];
            newPlayerMsg.msg.head.seqNo = buf[1];
            newPlayerMsg.msg.head.id = buf[2];
            newPlayerMsg.msg.head.type = MsgType.values()[buf[3]]; // should always be MsgType.Change
            newPlayerMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.NewPlayer

            newPlayerMsg.desc = ObjectDesc.values()[buf[5]];
            newPlayerMsg.form = ObjectForm.values()[buf[6]];

            seqNo = newPlayerMsg.msg.head.seqNo;
            if (id == 0) {
                id = newPlayerMsg.msg.head.id;
            }
            Client c = new Client();
            c.clientId = newPlayerMsg.msg.head.id;
            c.objectDesc = newPlayerMsg.desc;
            c.objectForm = newPlayerMsg.form;
            c.position.x = 101; // not set
            c.position.y = 101; // not set
            info.add(c);

            if (getClient(c.clientId) != null) {
                xCoordBuf.add((byte) c.position.x);
                yCoordBuf.add((byte) c.position.y);
                colorBuf.add((byte) ObjectForm.values()[buf[6]].ordinal());
            }
        }

        if ((byte)ChangeType.PlayerLeave.ordinal() == buf[4]){
            plLevMsgcount++;
            PlayerLeaveMsg playerLeaveMsg = new PlayerLeaveMsg();
            playerLeaveMsg.msg = new ChangeMsg();
            playerLeaveMsg.msg.head = new MsgHead();

            playerLeaveMsg.msg.head.length = buf[0];
            playerLeaveMsg.msg.head.seqNo = buf[1];
            playerLeaveMsg.msg.head.id = buf[2];
            playerLeaveMsg.msg.head.type = MsgType.values()[buf[3]]; // Should always be MsgType.Change
            playerLeaveMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.PlayerLeave

            for (Client c : info){
                if (c.clientId == playerLeaveMsg.msg.head.id){
                    info.remove(c);
                }
            }
        }

        if ((byte)ChangeType.NewPlayerPosition.ordinal() == buf[4]){
            newPlPosMsgcount++;
            NewPlayerPositionMsg newPlayerPositionMsg = new NewPlayerPositionMsg();
            newPlayerPositionMsg.msg = new ChangeMsg();
            newPlayerPositionMsg.msg.head = new MsgHead();
            newPlayerPositionMsg.pos = new Coordinate();

            newPlayerPositionMsg.msg.head.length = buf[0];
            newPlayerPositionMsg.msg.head.seqNo = buf[1];
            newPlayerPositionMsg.msg.head.id = buf[2];
            newPlayerPositionMsg.msg.head.type = MsgType.values()[buf[3]]; // should always be MsgType.Change
            newPlayerPositionMsg.msg.type = ChangeType.values()[buf[4]]; // should always be ChangeType.NewPlayerPosition

            newPlayerPositionMsg.pos.x = buf[5];
            newPlayerPositionMsg.pos.y = buf[6];

            for (Client curr : info) {
                if (buf[2] == curr.clientId) {
                    curr.position = new Coordinate();
                    curr.position.x = buf[5];
                    curr.position.y = buf[6];
                    System.out.println("\treceiving: " + "(" + (buf[5] + 100) + ", " + (buf[6] + 100) + ")");
                    xCoordBuf.add((byte) (buf[5] + 100));
                    yCoordBuf.add((byte) (buf[6] + 100));
                    colorBuf.add((byte)curr.objectDesc.ordinal());
                    // shape to print
                    switch (curr.objectForm.ordinal()){
//                        case ObjectForm.Cube.ordinal() ->{
//                            System.in.available();
//                        }
                    }
//                    System.out.println(seqNo);
//                    System.out.println(curr.position.x);
//                    System.out.println(curr.clientId);
//                    System.out.println(curr.objectDesc);
//                    System.out.println(curr.objectForm);
//                    System.out.println(curr.position.x);
//                    System.out.println(curr.position.y);

                }
            }

//            System.out.println(newPlayerPositionMsg.msg.head.length);
//            System.out.println(newPlayerPositionMsg.msg.head.seqNo);
//            System.out.println(newPlayerPositionMsg.msg.head.id);
//            System.out.println(newPlayerPositionMsg.msg.head.type); // should always be MsgType.Change
//            System.out.println(newPlayerPositionMsg.msg.type); // should always be ChangeType.NewPlayerPosition
//            System.out.println(newPlayerPositionMsg.pos.x);
//            System.out.println(newPlayerPositionMsg.pos.y);

//            xCoordBuf.add((byte)newPlayerPositionMsg.pos.x);
//            xCoordBuf.add((byte)newPlayerPositionMsg.pos.y);
        }

        //find the client that needs to be redrawn otherwise draw the newly joined client

//        xCoordBuf.clear();
//        yCoordBuf.clear();
//        colorBuf.clear();
//        for (int i = 0; i < info.size(); i++) {
//            Coordinate center = info.get(i).position;
//            int descColor = Fit(info.get(i).objectDesc.ordinal(), 16, 1);
//            switch (info.get(i).objectForm) {
//                case Cone, Pyramid -> {
//                    xCoordBuf.add((byte) (center.x));
//                    yCoordBuf.add((byte) (center.y + 1));
//                    colorBuf.add((byte)descColor);
//
//                    xCoordBuf.add((byte) (center.x - 1));
//                    yCoordBuf.add((byte) (center.y));
//                    colorBuf.add((byte)descColor);
//
//                    xCoordBuf.add((byte) (center.x));
//                    yCoordBuf.add((byte) (center.y));
//                    colorBuf.add((byte)descColor);
//
//                    xCoordBuf.add((byte) (center.x + 1));
//                    yCoordBuf.add((byte) (center.y));
//                    colorBuf.add((byte)descColor);
//
//                    xCoordBuf.add((byte) (center.x));
//                    yCoordBuf.add((byte) (center.y - 1));
//                    colorBuf.add((byte)descColor); // romb
//                }
//            }
//        }

        //Reformat
//        for (int k = 0; k < byteBuf.size(); k += 3) {
//            info.add(
//                    Byte.toUnsignedInt(byteBuf.get(k)),
//                    Byte.toUnsignedInt(byteBuf.get(k + 1)),
//                    Byte.toUnsignedInt(byteBuf.get(k + 2)))); // always a multiple of three
//            System.out.print(
//                    (byteBuf.get(k)+(byte)48) +
//                    (byteBuf.get(k+1)+(byte)48) +
//                    (byteBuf.get(k+2)+(byte)48));
//            info.clear();
//            info.add(new Pixel(Byte.toUnsignedInt(byteBuf.get(k)), Byte.toUnsignedInt(byteBuf.get(k+1)), Byte.toUnsignedInt(byteBuf.get(k+2))));
//        }
        window.canvas.SetParamArr(xCoordBuf, yCoordBuf, colorBuf);
        window.frame.repaint();
    }

    public static void speak() throws IOException
    {
        byte[] pl = new byte[7];

        pl[0] = 7;
        pl[1] = (byte) ++seqNo;
        pl[2] = (byte)id;
        pl[3] = (byte)MsgType.Event.ordinal();
        pl[4] = (byte)EventType.Move.ordinal();
        pl[5] = (byte)getClient(id).position.x;
        pl[6] = (byte)getClient(id).position.y;

        //new position
        switch (direction)
        {
            case -1:
                break;
            case 0:
                pl[6] -= 1;
                break;
            case 1:
                pl[6] += 1;
                break;
            case 2:
                pl[5] -= 1;
                break;
            case 3:
                pl[5] += 1;
                break;
            default:
                System.out.println("Error in speak() switch");
                return;
        }

        //System.out.println(pl);
        System.out.print("sending: (" + pl[5] + ", " + pl[6] + ")");
        out.write(pl);
    }


    public static void main(String[] args) throws IOException {
        GUI window = new GUI();
        runSetup(4999);
        while(true) {
            // no specific protocol needs to be used
            try {
                listen(window);
            }catch (SocketException s){
                s.printStackTrace();
                System.exit(1);
            }
        }
    }
}