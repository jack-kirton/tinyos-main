package net.tinyos.packet;

import java.util.HashMap;

class Platform {
    static int x;
    static HashMap<String, Integer> platforms;

    static void add(int dummy, String name, int baudrate) {
        platforms.put(name, baudrate);
    }

    static int get(String name) {
        if (platforms == null) {
            platforms = new HashMap<>();
            try {
              BaudRate.init();
            }
            catch (Exception e) {
              System.err.println("Failed to initialize baud rates for platforms. Serial communication may not work properly.");
            }
        }
        Integer val = platforms.get(name);

        if (val != null)
            return val.intValue();
        else
            return -1;
    }
}
