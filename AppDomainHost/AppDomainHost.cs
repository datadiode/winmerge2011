using System;
using System.Collections.Generic;

namespace AppDomainHost
{
    public static class AppDomainHost
    {
        static Lazy<AppDomain> domSQLiteCompare = new Lazy<AppDomain>(() =>
            AppDomain.CreateDomain("SQLiteCompare", null,
            AppDomain.CurrentDomain.BaseDirectory + "SQLiteCompare\\bin\\",
            null, false));

        static Lazy<AppDomain> domReoGridCompare = new Lazy<AppDomain>(() =>
            AppDomain.CreateDomain("ReoGridCompare", null,
            AppDomain.CurrentDomain.BaseDirectory + "ReoGridCompare\\bin\\",
            null, false));

        public static int SQLiteCompare(string args) =>
            ExecuteAssembly(domSQLiteCompare.Value, "SQLiteCompare.exe", args);

        public static int ReoGridCompare(string args) =>
            ExecuteAssembly(domReoGridCompare.Value, "ReoGridCompare.exe", args);

        /// <summary>
        /// https://github.com/adamabdelhamed/PowerArgs
        /// Copyright (c) 2013 Adam Abdelhamed
        /// SPDX-License-Identifier: MIT
        /// Converts a single string that represents a command line to be executed into a string[], 
        /// accounting for quoted arguments that may or may not contain spaces.
        /// </summary>
        /// <param name="commandLine">The raw arguments as a single string</param>
        /// <returns>a converted string array with the arguments properly broken up</returns>
        static string[] SplitArgs(string commandLine)
        {
            List<string> ret = new List<string>();
            string currentArg = string.Empty;
            bool insideDoubleQuotes = false;

            for (int i = 0; i < commandLine.Length; i++)
            {
                var c = commandLine[i];

                if (insideDoubleQuotes && c == '"')
                {
                    ret.Add(currentArg);
                    currentArg = string.Empty;
                    insideDoubleQuotes = !insideDoubleQuotes;
                }
                else if (!insideDoubleQuotes && c == ' ')
                {
                    if (currentArg.Length > 0)
                    {
                        ret.Add(currentArg);
                        currentArg = string.Empty;
                    }
                }
                else if (c == '"')
                {
                    insideDoubleQuotes = !insideDoubleQuotes;
                }
                else if (c == '\\' && i < commandLine.Length - 1 && commandLine[i + 1] == '"')
                {
                    currentArg += '"';
                    i++;
                }
                else
                {
                    currentArg += c;
                }
            }

            if (currentArg.Length > 0)
            {
                ret.Add(currentArg);
            }

            return ret.ToArray();
        }

        static int ExecuteAssembly(AppDomain dom, string path, string args) =>
            dom.ExecuteAssembly(dom.BaseDirectory + path, SplitArgs(args));
    }
}
