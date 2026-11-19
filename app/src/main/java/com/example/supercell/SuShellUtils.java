package com.example.supercell;

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class SuShellUtils {


    public static ExecResult execSuCommand(String command) throws Exception {
        ProcessBuilder pb = new ProcessBuilder("su", "-c", command);
        Process process = pb.start();

        BufferedReader stdoutReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
        StringBuilder stdoutBuilder = new StringBuilder();
        String line;
//        readLine会剔除行尾换行\r,\n,\r\n
        while ((line = stdoutReader.readLine()) != null) {
            stdoutBuilder.append(line).append(System.lineSeparator());
        }

        BufferedReader stderrReader = new BufferedReader(new InputStreamReader(process.getErrorStream()));
        StringBuilder stderrBuilder = new StringBuilder();
        while ((line = stderrReader.readLine()) != null) {
            stderrBuilder.append(line).append(System.lineSeparator());
        }
        int exitCode = process.waitFor();
        return new ExecResult(exitCode, stdoutBuilder.toString(), stderrBuilder.toString());
    }

    //    不可重新赋值的数据对象
    public record ExecResult(int exitCode, String stdout, String stderr) {
    }
}
