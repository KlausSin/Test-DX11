import json
import re
import pathlib
import glob

def vec(line):
    return [float(x) for x in re.findall(r'-?\d+\.\d+|-?\d+', line)]

def quoted(line):
    m = re.search(r'"(.*?)"', line)
    return m.group(1).replace("\\", "/") if m else ""

for file in glob.glob("*.msenv"):

    with open(file, "r", encoding="utf-8", errors="ignore") as f:
        lines = [x.strip() for x in f.readlines()]

    data = {
        "reserved": False,

        "directionallight": {
            "direction": [],
            "background": {},
            "character": {}
        },

        "material": {},

        "fog": {},

        "filter": {},

        "skybox": {
            "gradient": []
        },

        "lensflare": {}
    }

    i = 0

    while i < len(lines):

        line = lines[i]

        #
        # DIRECTION
        #
        if line.startswith("Direction"):
            data["directionallight"]["direction"] = vec(line)

        #
        # BACKGROUND
        #
        elif line.startswith("Enable"):
            ctx = "\n".join(lines[max(0, i - 5):i])

            if "Background" in ctx:
                data["directionallight"]["background"]["enable"] = bool(int(vec(line)[0]))

            elif "Character" in ctx:
                data["directionallight"]["character"]["enable"] = bool(int(vec(line)[0]))

            elif "Group Filter" in ctx:
                data["filter"]["enable"] = bool(int(vec(line)[0]))

            elif "Group LensFlare" in ctx:
                data["lensflare"]["enable"] = bool(int(vec(line)[0]))

        elif line.startswith("Diffuse"):

            vals = vec(line)
            ctx = "\n".join(lines[max(0, i - 5):i])

            if "Background" in ctx:
                data["directionallight"]["background"]["diffuse"] = vals

            elif "Character" in ctx:
                data["directionallight"]["character"]["diffuse"] = vals

            elif "Group Material" in ctx:
                data["material"]["diffuse"] = vals

        elif line.startswith("Ambient"):

            vals = vec(line)
            ctx = "\n".join(lines[max(0, i - 5):i])

            if "Background" in ctx:
                data["directionallight"]["background"]["ambient"] = vals

            elif "Character" in ctx:
                data["directionallight"]["character"]["ambient"] = vals

            elif "Group Material" in ctx:
                data["material"]["ambient"] = vals

        elif line.startswith("Emissive"):
            data["material"]["emissive"] = vec(line)

        #
        # FOG
        #
        elif line.lower().startswith("foglevel"):
            data["fog"]["foglevel"] = int(vec(line)[0])

        elif line.startswith("Color"):

            vals = vec(line)
            ctx = "\n".join(lines[max(0, i - 5):i])

            if "Group Fog" in ctx:
                data["fog"]["color"] = vals

            elif "Group Filter" in ctx:
                data["filter"]["color"] = vals

        #
        # FILTER
        #
        elif line.startswith("AlphaSrc"):
            data["filter"]["alphasrc"] = int(vec(line)[0])

        elif line.startswith("AlphaDest"):
            data["filter"]["alphadest"] = int(vec(line)[0])

        #
        # SKYBOX
        #
        elif line.startswith("bTextureRenderMode"):
            data["skybox"]["btexturerendermode"] = bool(int(vec(line)[0]))

        elif line.startswith("Scale"):
            data["skybox"]["scale"] = vec(line)

        elif line.startswith("GradientLevelUpper"):
            data["skybox"]["gradientlevelupper"] = int(vec(line)[0])

        elif line.startswith("GradientLevelLower"):
            data["skybox"]["gradientlevellower"] = int(vec(line)[0])

        elif line.startswith("FrontFaceFileName"):
            data["skybox"]["frontfacefilename"] = quoted(line)

        elif line.startswith("BackFaceFileName"):
            data["skybox"]["backfacefilename"] = quoted(line)

        elif line.startswith("LeftFaceFileName"):
            data["skybox"]["leftfacefilename"] = quoted(line)

        elif line.startswith("RightFaceFileName"):
            data["skybox"]["rightfacefilename"] = quoted(line)

        elif line.startswith("TopFaceFileName"):
            data["skybox"]["topfacefilename"] = quoted(line)

        elif line.startswith("BottomFaceFileName"):
            data["skybox"]["bottomfacefilename"] = quoted(line)

        elif line.startswith("CloudScale"):
            data["skybox"]["cloudscale"] = vec(line)

        elif line.startswith("CloudHeight"):
            data["skybox"]["cloudheight"] = vec(line)[0]

        elif line.startswith("CloudTextureScale"):
            data["skybox"]["cloudtexturescale"] = vec(line)

        elif line.startswith("CloudSpeed"):
            data["skybox"]["cloudspeed"] = vec(line)

        elif line.startswith("CloudTextureFileName"):
            data["skybox"]["cloudtexturefilename"] = quoted(line)

        #
        # CLOUD COLOR
        #
        elif line.startswith("List CloudColor"):

            vals = []

            i += 2

            while not lines[i].startswith("}"):

                vals.extend(vec(lines[i]))
                i += 1

            data["skybox"]["cloudcolor"] = vals

        #
        # GRADIENT
        #
        elif line.startswith("List Gradient"):

            grads = []

            i += 2

            temp = []

            while not lines[i].startswith("}"):

                if lines[i]:

                    temp.extend(vec(lines[i]))

                    if len(temp) == 8:
                        grads.append(temp)
                        temp = []

                i += 1

            data["skybox"]["gradient"] = grads

        #
        # LENSFLARE
        #
        elif line.startswith("BrightnessColor"):
            data["lensflare"]["brightnesscolor"] = vec(line)

        elif line.startswith("MaxBrightness"):
            data["lensflare"]["maxbrightness"] = vec(line)[0]

        elif line.startswith("MainFlareEnable"):
            data["lensflare"]["mainflareenable"] = bool(int(vec(line)[0]))

        elif line.startswith("MainFlareTextureFileName"):
            data["lensflare"]["mainflaretexturefilename"] = quoted(line)

        elif line.startswith("MainFlareSize"):
            data["lensflare"]["mainflaresize"] = vec(line)[0]

        i += 1

    out = pathlib.Path(file).with_suffix(".json")

    with open(out, "w", encoding="utf-8") as f:
        txt = json.dumps(data, indent=4)

        txt = re.sub(
            r'\[\s+([^\[\]]+?)\s+\]',
            lambda m: '[' + ' '.join(m.group(1).split()) + ']',
            txt,
            flags=re.MULTILINE
        )
        
        f.write(txt)

    print("Converted:", file)
    