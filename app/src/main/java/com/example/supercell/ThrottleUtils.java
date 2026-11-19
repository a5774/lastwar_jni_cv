package com.example.supercell;

import java.util.HashMap;
import java.util.Map;

/** @noinspection DataFlowIssue*/
public class ThrottleUtils {
    private static final Map<String, Long> lastCallTimeMap = new HashMap<>();
    public static void runThrottled(String key, long interval, Runnable action) throws NullPointerException {
        long now = System.currentTimeMillis();
        Long lastTime = lastCallTimeMap.getOrDefault(key, 0L);
        if (now - lastTime >= interval) {
            lastCallTimeMap.put(key, now);
            action.run();
        }
    }
}
