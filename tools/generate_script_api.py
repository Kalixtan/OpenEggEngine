#!/usr/bin/env python3
"""Build a GameMonkeyScript API inventory from the binding registrations."""

from collections import defaultdict
import csv
import json
from hashlib import sha1
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
BINDINGS = ROOT / "src/bindings"
SCRIPT_API_SOURCES = sorted((ROOT / "src").rglob("*.cpp"))

TABLE_START = re.compile(r'gmFunctionEntry\s+([A-Za-z_]\w*)\s*\[\s*\]\s*=\s*\{')
TABLE_ENTRY = re.compile(r'\{\s*"([^"\n]+)"\s*,\s*([A-Za-z_]\w*)')
TYPE_LIBRARY = re.compile(r'RegisterTypeLibrary\s*\(\s*[^,]+,\s*([A-Za-z_]\w*)')
TYPE_REGISTRATION = re.compile(
    r'RegisterTypeLibrary\s*\(\s*([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)'
)
CUSTOM_TYPE_SYMBOL = re.compile(r"\bgmType\s*&?\s*(g_[A-Za-z_]\w*)\s*=")
CUSTOM_TYPE_ALIAS = re.compile(r"\bgmType\s*&\s*(g_[A-Za-z_]\w*)\s*=\s*(g_[A-Za-z_]\w*)")
CUSTOM_TYPE_CREATE = re.compile(
    r'\b(g_[A-Za-z_]\w*)\s*=\s*a_machine\.CreateUserType\s*\(\s*"([^"]+)"'
)
LIBRARY_REGISTRATION = re.compile(
    r'RegisterLibrary\s*\(\s*([A-Za-z_]\w*)\s*,\s*[^,]+,\s*"([^"]+)"'
)
DIRECT_ENTRY = re.compile(
    r'RegisterLibraryFunction\s*\(\s*"([^"\n]+)"\s*,\s*([A-Za-z_]\w*)'
    r'(?:\s*,\s*"([^"\n]+)")?'
)
GLOBAL_FUNCTION_REGISTRATION = re.compile(
    r'\b(?:a_machine|machine)\.GetGlobals\(\)->Set\s*\(\s*&\s*'
    r'(?:a_machine|machine)\s*,\s*"([^"\n]+)"\s*,\s*'
    r'gmVariable\s*\(\s*(?:a_machine|machine)\.AllocFunctionObject\s*\('
    r'\s*&?\s*([A-Za-z_]\w*)\s*\)\s*\)\s*\)', re.DOTALL
)
ENTITY_INSTANCE_REGISTRATION = re.compile(
    r'a_table->Set\s*\(\s*&a_machine\s*,\s*"([^"\n]+)"\s*,\s*'
    r'gmVariable\s*\(\s*a_machine\.AllocFunctionObject\s*\(\s*&?'
    r'([A-Za-z_]\w*)\s*\)', re.DOTALL
)
SET_NATIVE_IF_MISSING = re.compile(
    r'setNativeIfMissing\s*\(\s*"([^"\n]+)"\s*,\s*'
    r'([A-Za-z_]\w*)\s*\)', re.DOTALL
)
TABLE_FUNCTION_REGISTRATION = re.compile(
    r'\w+->Set\s*\(\s*&(?:a_machine|m)\s*,\s*"([^"\n]+)"\s*,\s*'
    r'gmVariable\s*\(\s*(?:a_machine|m)\.AllocFunctionObject\s*\(\s*&?'
    r'([A-Za-z_]\w*)\s*\)', re.DOTALL
)
TABLE_VALUE_FUNCTION_REGISTRATION = re.compile(
    r'\w+->Set\s*\(\s*&a_machine\s*,\s*"([^"\n]+)"\s*,\s*'
    r'(g_getSizeFn|g_getFilenameFn|g_getIfDoneFn|g_getIfErrorFn)\s*\)'
)
HANDLER_DEF = re.compile(
    r"(?:^|\n)\s*(?:int|void|bool|gmVariable|gmType|float|double|"
    r"[A-Za-z_]\w*\s*\*)\s+(?:GM_CDECL\s+)?([A-Za-z_]\w*)\s*\([^;]*?\)\s*\{",
    re.MULTILINE,
)
PARAM_MACRO = re.compile(
    r"\b(GM_[A-Z_]+_PARAM|GM_[A-Z_]+_OR_[A-Z_]+_PARAM)\s*\(\s*"
    r"([A-Za-z_]\w*)\s*,\s*([^,()]+)\s*(?:,\s*([^)]*))?"
)
USER_PARAM_MACRO = re.compile(
    r"\bGM_(CHECK_)?USER_PARAM\s*\(\s*([^,]+),\s*([A-Za-z_]\w*),"
    r"\s*([A-Za-z_]\w*)\s*,\s*(\d+)\s*\)"
)
PARAM_ASSIGNMENT = re.compile(
    r"\b(?:(?:(?:const|volatile)\s+)?(auto|gmVariable|gmint|gmfloat|int|float|double|"
    r"bool|unsigned\s+int|const\s+char\s*\*|gmTableObject\s*\*)\s*[*&]*\s*)?"
    r"([A-Za-z_]\w*)\s*=\s*(?:\(\s*(?:int|float)\s*\)\s*)?"
    r"(?:a_thread|thread)->Param(FloatOrInt|Int|Float|String|Table)?\s*\(\s*(\d+)"
)
PARAM_CONTEXT_ASSIGNMENT = re.compile(
    r"\b(?:(?:const|volatile|static)\s+)*(gmVariable|gmTableObject\s*\*|"
    r"gmint|gmfloat|std::string|unsigned\s+int|int|float|double|bool|auto)"
    r"\s*[*&]*\s+([A-Za-z_]\w*)\s*=\s*[^;]*?"
    r"(?:a_thread|thread)->Param(FloatOrInt|Int|Float|String|Table)?"
    r"\s*\(\s*(\d+)")
PUSH_CALL = re.compile(r"\b(a_thread|thread)->(Push[A-Za-z0-9_]*)\s*\(")
CHECK_PARAMS = re.compile(r"\bGM_CHECK_NUM_PARAMS\s*\(\s*(\d+)\s*\)")
PARAM_TYPES = {
    "GM_INT_PARAM": "int",
    "GM_FLOAT_OR_INT_PARAM": "number",
    "GM_NUM_PARAM": "number",
    "GM_STRING_PARAM": "string",
    "GM_TABLE_PARAM": "table",
    "GM_CHECK_TABLE_PARAM": "table",
    "GM_CHECK_USER_PARAM": "object",
    "GM_CHECK_NUM_PARAM": "number",
}
PUSH_TYPES = {
    "PushInt": "int",
    "PushFloat": "number",
    "PushNewString": "string",
    "PushNull": "null",
    "PushTable": "table",
    "PushNewUser": "object",
    "PushVariable": "value",
    "PushBool": "bool",
    "Push": "value",
}
WORDS = re.compile(r"[A-Z]+(?=[A-Z][a-z]|\b)|[A-Z]?[a-z]+|[0-9]+")
WORD_EXPANSIONS = {
    "pos": "position", "vel": "velocity", "rot": "rotation",
    "ang": "angular", "dir": "direction", "rad": "radians",
    "deg": "degrees", "idx": "index", "num": "number",
    "perc": "percentage", "ref": "reference", "info": "information",
    "tex": "texture", "anim": "animation", "phys": "physics",
    "mat": "matrix", "col": "colour", "str": "string",
    "obj": "object", "freq": "frequency", "amp": "amplitude",
    "ms": "milliseconds", "id": "ID",
}
VERBS = {
    "get": "Returns",
    "is": "Reports whether",
    "has": "Reports whether",
    "can": "Reports whether",
    "set": "Sets",
    "create": "Creates",
    "make": "Creates or formats",
    "new": "Creates",
    "destroy": "Destroys",
    "delete": "Removes",
    "remove": "Removes",
    "add": "Adds",
    "clear": "Clears",
    "reset": "Resets",
    "start": "Starts",
    "stop": "Stops",
    "begin": "Begins",
    "end": "Ends",
    "load": "Loads",
    "save": "Saves",
    "draw": "Draws",
    "play": "Starts playback of",
    "pause": "Pauses",
    "unpause": "Resumes",
    "enable": "Enables",
    "disable": "Disables",
    "convert": "Converts",
    "find": "Finds",
    "calculate": "Calculates",
    "compute": "Calculates",
    "update": "Updates",
    "init": "Initializes",
    "append": "Appends to",
    "insert": "Inserts into",
    "sort": "Sorts",
    "format": "Formats",
    "open": "Opens",
    "close": "Closes",
    "read": "Reads",
    "write": "Writes",
    "register": "Registers",
    "unregister": "Unregisters",
    "connect": "Connects",
    "disconnect": "Disconnects",
    "trigger": "Triggers",
    "run": "Runs",
    "quit": "Quits",
    "trace": "Traces",
    "dump": "Dumps",
    "copy": "Copies",
    "paste": "Pastes",
    "compare": "Compares",
    "count": "Counts",
    "contains": "Checks whether the value contains",
    "test": "Tests",
    "getall": "Returns all",
    "getnum": "Returns the number of",
    "send": "Sends",
    "post": "Posts",
    "request": "Requests",
    "query": "Queries",
    "move": "Moves",
    "rotate": "Rotates",
    "translate": "Translates",
    "scale": "Scales",
    "push": "Pushes",
    "pop": "Removes the top",
    "bind": "Binds",
    "free": "Frees",
    "break": "Breaks",
    "goto": "Moves to",
    "go": "Moves",
    "put": "Places",
    "replace": "Replaces",
    "merge": "Merges",
    "purge": "Purges",
    "probe": "Probes",
    "print": "Prints",
    "debug": "Runs the debug operation for",
    "profile": "Records a profiling marker for",
    "pulse": "Pulses",
    "shade": "Applies shading to",
    "brake": "Applies braking to",
    "raycast": "Casts a ray against",
    "distance": "Returns the distance to",
    "length": "Returns the length of",
    "size": "Returns the size of",
    "normalize": "Normalizes",
    "clamp": "Clamps",
    "lerp": "Interpolates between",
    "interpolate": "Interpolates",
    "serialize": "Serializes",
    "deserialize": "Deserializes",
    "encode": "Encodes",
    "decode": "Decodes",
    "copy": "Copies",
    "paste": "Pastes",
    "subtract": "Subtracts",
    "ensure": "Ensures",
    "resume": "Resumes",
    "pause": "Pauses",
    "replacebody": "Replaces the body for",
    "dummy": "Provides a placeholder operation for",
    "unload": "Unloads",
    "launch": "Launches",
    "plot": "Adds a vertex to",
    "pack": "Packs",
    "unpack": "Unpacks",
    "flush": "Flushes",
    "suppress": "Suppresses",
    "restore": "Restores",
    "assert": "Checks the assertion",
    "thrust": "Applies thrust to",
    "reverse": "Reverses",
    "attach": "Attaches",
    "calc": "Calculates",
    "to": "Converts to",
    "from": "Creates from",
    "finish": "Finishes",
    "check": "Checks",
    "restart": "Restarts",
    "take": "Captures",
    "screenshot": "Captures a screenshot of",
    "virtual": "Returns",
    "left": "Returns",
    "save": "Saves",
    "request": "Requests",
    "does": "Reports whether",
    "do": "Performs",
    "localize": "Localizes",
    "generate": "Generates",
    "apply": "Applies",
    "release": "Releases",
    "rebuild": "Rebuilds",
    "wake": "Wakes",
    "sleep": "Puts to sleep",
    "suppress": "Suppresses",
    "backspace": "Removes the previous character from",
    "stream": "Streams",
    "initialize": "Initializes",
    "initialise": "Initializes",
    "setas": "Sets",
    "hsl": "Converts HSL values to",
}


def body_for(source: str, handler: str):
    for match in HANDLER_DEF.finditer(source):
        if match.group(1) != handler:
            continue
        opening = source.find("{", match.start())
        depth = 0
        for pos in range(opening, len(source)):
            if source[pos] == "{":
                depth += 1
            elif source[pos] == "}":
                depth -= 1
                if depth == 0:
                    return (source[opening + 1:pos],
                            source.count("\n", 0, match.start()) + 1,
                            doc_before(source, match.start()))
        break
    return "", 0, ""


def doc_before(source: str, start: int):
    close = source.rfind("*/", 0, start)
    if close < 0 or source[close + 2:start].strip():
        return ""
    opening = source.rfind("/**", 0, close)
    if opening < 0:
        return ""
    lines = source[opening + 3:close].splitlines()
    result = []
    for line in lines:
        line = re.sub(r"^\s*\* ?", "", line).strip()
        if not line or line.startswith("@ingroup"):
            continue
        if line.startswith("@brief"):
            line = line[len("@brief"):].strip()
        elif line.startswith("@param") or line.startswith("@return"):
            line = line[1:]
        elif line.startswith("@"):
            continue
        result.append(line)
    return " ".join(result)


def table_entries(source: str):
    found = []
    for match in TABLE_START.finditer(source):
        opening = source.find("{", match.start())
        depth = 0
        for pos in range(opening, len(source)):
            if source[pos] == "{":
                depth += 1
            elif source[pos] == "}":
                depth -= 1
                if depth == 0:
                    found.extend((match.group(1), name, handler)
                                 for name, handler in TABLE_ENTRY.findall(source[opening + 1:pos]))
                    break
    return found


def owner_name(table: str, source: str):
    for registered in TYPE_LIBRARY.findall(source):
        if registered == table:
            return re.sub(r"^s_|Lib$", "", table)
    for registered, library_name in LIBRARY_REGISTRATION.findall(source):
        if registered == table:
            return library_name
    return "Global"


def human_words(value: str):
    return " ".join(word.lower() for word in WORDS.findall(value))


def inferred_description(name: str, handler: str):
    constructors = {"COLOUR", "ENTITYLISTREFCONTAINER", "ENVELOPE", "FILE",
                    "INSTRUMENT", "INTLISTREFCONTAINER", "MATRIX4",
                    "PHYSICSSHAPECOLLECTIONDEF", "PULSEPERC", "SEQUENCER",
                    "SEQUENCERACTION", "SHAPE", "TRIANGLELFO"}
    if name in constructors:
        label = human_words(name)
        return f"Creates a `{label}` value."
    if name == "PRINTINFO":
        return "Prints diagnostic information about the supplied value."
    if name in ("PROFILE_MARKER_PUSH_PROCESS", "PROFILE_MARKER_POP"):
        return "Adds or removes a profiling marker for the current process."
    tokens = WORDS.findall(name.replace("_", " "))
    start = 1 if "_" in name else 0
    tokens = tokens[start:]
    action = ""
    target_words = []
    for index, token in enumerate(tokens):
        candidate = token.casefold()
        if candidate in VERBS:
            action = candidate
            target_words = tokens[index + 1:]
            if action == "get" and target_words and target_words[0].casefold() in ("is", "has", "can"):
                action = target_words.pop(0).casefold()
            break
        if candidate in ("float2int", "int2float", "degtorad"):
            action = candidate
            break
        for verb in sorted(VERBS, key=len, reverse=True):
            if candidate.startswith(verb) and len(candidate) > len(verb):
                action = verb
                target_words = [candidate[len(verb):], *tokens[index + 1:]]
                break
            if candidate.endswith(verb) and len(candidate) > len(verb) and verb in ("print", "break", "assert"):
                action = verb
                target_words = tokens[:index] + [candidate[:-len(verb)]]
                break
        if action:
            break
    if action in ("float2int", "int2float", "degtorad"):
        target_words = {"float2int": ["float", "to", "integer"],
                        "int2float": ["integer", "to", "float"],
                        "degtorad": ["degrees", "to", "radians"]}[action]
        action = "convert"
    if not action:
        words = " ".join(human_words(part) for part in name.split("_")).strip()
        if name.casefold() == "test":
            return "Checks that the receiver is a valid native object."
        return f"Performs {words or name}."
    target = " ".join(WORD_EXPANSIONS.get(word.casefold(), word.lower())
                      for word in target_words).strip()
    if action in ("is", "has", "can"):
        return f"Reports whether {target or 'the requested condition'} is true."
    if action in ("get", "getall", "getnum"):
        return f"{VERBS[action]} {target or 'the value requested by this binding'}."
    if action == "set":
        return f"Sets the value of {target or 'the requested property'}."
    if action in VERBS:
        return f"{VERBS[action]} {target or 'the current value'}."
    return f"Performs the `{name}` operation through `{handler}`."


def parameter_description(name: str, method: str = ""):
    key = re.sub(r"[^a-z]", "", name.casefold())
    descriptions = {
        "x": "X coordinate or component.", "y": "Y coordinate or component.",
        "z": "Z coordinate or depth value.", "w": "Width or fourth component; see the handler.",
        "width": "Width value.", "height": "Height value.",
        "id": "Identifier of the object or resource.",
        "index": "Index used by this operation; indexing follows the handler.",
        "name": "Name used by this operation.", "filename": "File or resource name.",
        "filepath": "Path to a file or resource.", "path": "Path to a file or resource.",
        "enabled": "Whether the behavior is enabled.", "enable": "Whether to enable the behavior.",
        "safe": "Whether safe-save behavior is enabled.", "paused": "Whether playback or processing is paused.",
        "value": "Value supplied to the operation.", "v": "Value supplied to the operation.",
        "str": "Input string.", "string": "Input string.", "text": "Text to process.",
        "delta": "Elapsed-time or change value; units follow the handler.",
        "deltams": "Elapsed time in milliseconds.", "ms": "Time in milliseconds.",
        "milliseconds": "Time in milliseconds.", "angle": "Angle value; units follow the handler.",
        "radians": "Angle in radians.", "degrees": "Angle in degrees.",
        "colour": "Colour value.", "color": "Colour value.",
        "volume": "Volume level.", "speed": "Speed value.", "scale": "Scale value.",
        "count": "Number of items.", "length": "Length value.", "size": "Size value.",
        "self": "Object receiving the operation.", "table": "Table supplied to the operation.",
        "character": "Character value, supplied as an integer code or a one-character string.",
        "safe": "Whether safe-save behavior is enabled.", "arg1": "First argument supplied to the method.",
        "arg2": "Second argument supplied to the method.", "arg3": "Third argument supplied to the method.",
        "vector1": "First 2D vector argument.", "vector2": "Second 2D vector argument.",
        "vector3": "Third 2D vector argument.", "vectororx": "A 2D vector, or the X component when coordinates are passed separately.",
        "voice": "Voice identifier returned by an audio playback method.",
        "voiceid": "Voice identifier returned by an audio playback method.",
        "position": "2D position in the coordinate space used by this operation.",
        "cell": "Screen-cell coordinate checked against the entity's cached screen-cell bounds.",
        "record": "Animation record index used by the playback operation.",
        "animation": "Animation handle to unload from the entity.",
        "entityarg": "Entity table passed to the operation.",
        "entityvar": "Entity table passed to the operation.",
        "durationarg": "Duration value used by this operation; units depend on the handler.",
        "alphaarg": "Alpha opacity value.",
        "noteArg": "MIDI note number used to set playback pitch.",
        "othervar": "Other entity table used as the target of this entity query.",
        "sourcevar": "Source entity table used by the relation query.",
        "targetvar": "Target entity table used by the relation query.",
        "loorhi": "Upper bound in the one-argument form; lower bound when both bounds are supplied.",
        "curve": "Exponent controlling the powered sine or cosine curve.",
        "refreshgrounddrawlistsonchange": "Whether ground draw lists are refreshed when entity registrations change.",
        "offset": "Integer offset added to the note index selected from the process scale.",
        "velocity": "MIDI velocity supplied for instrument playback.",
        "style": "Ground rendering style identifier.",
        "selected": "Whether this ground registration is selected.",
        "bits": "Description bit field stored on the entity.",
        "slot": "Shader parameter slot index.",
        "channelid": "Mixer channel identifier used with the receiving entity's mixer ID.",
        "newuidf": "New script UID, converted to an integer by truncation.",
        "intervalms": "Alarm interval in milliseconds.",
        "startready": "Whether the alarm starts with elapsed time equal to its interval.",
        "screenx": "Horizontal screen-cell coordinate.",
        "screeny": "Vertical screen-cell coordinate.",
        "extraz": "Additional Z depth offset applied while drawing.",
        "uvscaleu": "Scale applied to the texture's U coordinate.",
        "uvscalev": "Scale applied to the texture's V coordinate.",
        "code": "Key code passed to the key lookup or conversion function.",
        "localindex": "Zero-based index of the shape in the receiving physics body.",
        "group": "Collision group value assigned to the body.",
        "maximum": "Maximum motor force or torque, according to the joint type.",
        "flag": "Boolean marker flag; nonzero enables the marker state.",
        "align": "Memory alignment in bytes requested for the draw-list buffer.",
        "cc": "Collision category mask used to filter raycast hits.",
        "myidx": "Index of the receiving contact or collision entry.",
        "ptidx": "Index of the contact point to query.",
        "deg": "Angle in degrees.", "ang": "Angle in radians.",
        "colourname": "Name of the palette colour to update.",
        "bpmarg": "Tempo in beats per minute.",
        "playtime": "Recorded play time value used when packing the score.",
        "active": "Whether the process should be marked active.",
        "hidden": "Whether the animation is hidden.",
        "reversed": "Whether the animation playback direction is reversed.",
        "overlayvar": "Overlay animation value to attach to this animation.",
        "animvar": "Animation value to draw.",
        "levelid": "Identifier of the level to look up.",
        "dir": "Directory path to list.",
        "from": "First vertex index in the requested range.",
        "to": "Last vertex index in the requested range.",
        "py": "Vertical coordinate of the point to test.",
        "other": "Envelope whose state is copied into the receiving envelope.",
        "resource": "Resource path or name used to locate the stream.",
        "handle": "Handle identifying the sequencer trigger.",
        "slotarg": "Vertex shader parameter slot index.",
        "c": "Curve value stored by the envelope.",
        "keyvar": "Colour key or name used to select the palette entry.",
        "texvar": "Texture value used to generate or update the palette colour.",
        "sizearg": "Requested file size in bytes.",
        "request": "Save request object used to resolve the saved texture.",
        "result": "Output table that receives the colliding shape identifiers.",
        "out": "Output table populated with the intersection result.",
        "list": "Table of vectors whose centroid is calculated.",
        "alpha": "Alpha opacity value.",
        "func": "Stencil comparison function code.",
        "sfail__or_op_": "Stencil operation used when the stencil test fails.",
        "dpfail__or_ref_": "Stencil operation or reference value, depending on the overload.",
        "dppass_": "Stencil operation used when both stencil and depth tests pass.",
        "cameraRotationEnabled": "Whether camera rotation is enabled in the world view.",
        "note": "MIDI note number used to set the playback pitch.",
        "targetnote": "MIDI note number to reach.",
        "gate": "Playback gate duration in milliseconds; zero leaves the voice ungated.",
        "gatems": "Playback gate duration in milliseconds.",
        "pan": "Stereo pan value.", "volume": "Linear playback volume.",
        "vol": "Linear playback volume.", "priority": "Voice allocation priority.",
        "requestedpriority": "Voice allocation priority.",
        "effectchain": "Effect chain to attach to the voice.",
        "positionms": "Playback start position in milliseconds.",
        "startpositionms": "Playback start position in milliseconds.",
        "timefactor": "Tempo stretch factor applied to playback duration.",
        "noteoffset": "Pitch offset, in semitones, from the reference note.",
        "speed": "Playback rate multiplier.", "frequency": "Frequency value in hertz.",
        "h": "Hue component.", "s": "Saturation component.", "l": "Lightness component.",
        "start": "Start point of the vector or ray.", "end": "End point of the vector or ray.",
        "force": "Force vector to apply.", "anchora": "First joint anchor point.",
        "anchorb": "Second joint anchor point.", "point1": "First point in the operation.",
        "point2": "Second point in the operation.", "body1": "First physics body.",
        "body2": "Second physics body.", "source": "Source sequence table.",
        "destination": "Destination sequence table.", "texture": "Texture resource or handle.",
        "shader": "Shader object used by the operation.", "excludebodies": "Bodies to omit from the query result.",
        "reuseshape": "Optional Shape object to reuse for generated geometry.",
        "mode": "Mode value selecting the operation's behavior.",
        "modearg": "Mode value selecting the operation's behavior.",
        "shapeindex": "Index of the shape in the buffer or collection.",
        "vertexindex": "Index of the vertex in the shape.",
        "shapeid": "Identifier of the shape.", "othershapeid": "Identifier of the other shape.",
        "bodyindex": "Index of the body in the physics data.",
        "bodyid": "Identifier of the physics body.", "bodyidorindex": "Body identifier or index accepted by the handler.",
        "entityvar": "Entity table or value supplied to the operation.",
        "entityarg": "Entity value supplied to the operation.",
        "verticesvar": "Vertex list supplied to the operation.",
        "pointsvar": "Point list supplied to the operation.",
        "resultvar": "Output or reuse value supplied to the operation.",
        "reuse": "Object to reuse for the result when supported.",
        "filter": "Optional filter used to select matching objects.",
        "container": "Container object receiving or holding the values.",
        "mask": "Bit mask selecting the applicable flags or categories.",
        "category": "Category value used by the operation.",
        "sid": "Identifier of the sequencer or sequence.",
        "type": "Type value selecting the requested object or operation.",
        "typename": "Name of the object type.",
        "notetype": "Sequencer note type value.",
        "step": "Sequencer step index or count.",
        "frames": "Number of frames.", "frame": "Frame index.",
        "dt": "Elapsed time in seconds for this update.",
        "rate": "Rate used by the operation.",
        "delay": "Delay value in frames or milliseconds as specified by the handler.",
        "durationarg": "Duration value used by this operation; units depend on the handler.",
        "durationms": "Duration in milliseconds.",
        "radius": "Radius value.", "mass": "Mass value.", "friction": "Friction coefficient.",
        "restitution": "Restitution coefficient.", "capacity": "Maximum number of entries or vertices to reserve.",
        "wrap": "Whether the sequencer wraps after its final step.",
        "loop": "Whether playback or iteration repeats.",
        "reverse": "Whether playback runs in reverse.",
        "reset": "Whether to reset the animation or state before applying the operation.",
        "replaceexisting": "Whether to replace an existing entry with the same key.",
        "cleanupprevious": "Whether to clean up the previous value before replacement.",
        "centerx": "Rectangle center X coordinate.", "centery": "Rectangle center Y coordinate.",
        "halfwidth": "Half of the rectangle width.", "halfheight": "Half of the rectangle height.",
        "minx": "Minimum X coordinate.", "miny": "Minimum Y coordinate.",
        "maxx": "Maximum X coordinate.", "maxy": "Maximum Y coordinate.",
        "offsetx": "Horizontal offset.", "offsety": "Vertical offset.",
        "index1based": "One-based index of the item.",
        "axis": "Controller or vector axis index.", "joyindex": "Controller index.",
        "percent": "Percentage value, normally in the range 0–100.",
        "perc": "Interpolation amount, normally in the range 0–1.",
        "peak": "Peak value of the envelope or pulse.",
        "attack": "Attack time or rate supplied to the envelope.",
        "decay": "Decay time or rate supplied to the envelope.",
        "enabledarg": "Whether the feature is enabled.",
        "fullscreen": "Whether to use fullscreen mode.", "bitdepth": "Requested display colour depth in bits.",
        "bank": "Instrument or sample bank index.", "sequence": "Sequence table or sequence value.",
        "action": "Sequencer action value.", "pulse": "Pulse value or pulse generator.",
        "shader": "Shader object used by the operation.",
        "sid": "Identifier used to select the sequencer or sequence.",
        "arg": "Argument value passed to the operation.",
        "a": "First value or operand.", "b": "Second value or operand.",
        "lo": "Lower bound.", "hi": "Upper bound.",
        "r": "Radius or red component, as used by the method.",
        "g": "Green component, where this method accepts a colour.",
        "t": "Time or interpolation value used by this method.",
        "p": "Parameter value used by this operation.",
        "d": "Distance or delta value used by this operation.",
        "n": "Count or number value used by this operation.",
        "idx": "Index of the selected entry.", "sid": "Sequencer or sequence identifier.",
        "area": "Save-data area or resource area selected by this operation.",
        "content": "Text or data content to process.",
        "record": "Animation record index used by the playback operation.",
        "verticesvar": "Vertex list supplied to the operation.",
        "resultvar": "Existing result object or destination value.",
        "dest": "Destination object or location.",
        "destination": "Destination table or collection.",
        "dest_or_destination": "Destination table or collection.",
        "dstpath": "Destination resource path.", "srcpath": "Source resource path.",
        "srcarea": "Source save-data area.", "dstarea": "Destination save-data area.",
        "lo": "Lower limit for the requested range.", "hi": "Upper limit for the requested range.",
        "firstcost": "Initial profiler cost value.", "additionalcost": "Additional profiler cost value.",
        "perfmaxcost": "Maximum profiler cost threshold.",
        "gateSteps": "Gate duration in sequencer steps.",
        "gatestepsrand": "Randomized gate duration in sequencer steps.",
        "minpan": "Minimum stereo pan value.", "maxpan": "Maximum stereo pan value.",
        "minvol": "Minimum playback volume.", "maxvol": "Maximum playback volume.",
        "voicearg": "Voice identifier or value accepted by the handler.",
        "noteArg": "MIDI note number used to set playback pitch.",
        "positionarg": "Playback or object position supplied to the operation.",
        "posms": "Playback position in milliseconds.",
        "verticesvar": "Vertex list or vertex data supplied to the operation.",
        "pointsvar": "Point list or point data supplied to the operation.",
        "boundtable": "Table used to resolve the object or property bounds.",
        "entityvar": "Entity table passed to the operation.",
        "entityarg": "Entity table passed to the operation.",
        "enabledarg": "Nonzero value enables the feature; zero disables it.",
        "consideralreadycolliding": "Whether already-colliding bodies are included in the query.",
        "considersensors": "Whether sensor fixtures are included in the query.",
        "includesensors": "Whether sensor fixtures are included in the query.",
        "rejectcontainingbodypoint": "Whether to reject a ray hit when its origin is inside the body.",
        "myshapeid": "Identifier of the receiver's shape.",
        "othershapeid": "Identifier of the other shape.",
        "vertexcount": "Number of vertices in the shape.",
        "numverts": "Number of vertices in the shape.",
        "form": "Primitive or shape form value.",
        "shape": "Shape object supplied to the operation.",
        "shapeType": "Primitive or shape type code.",
        "typeName": "Name of the registered type.",
        "type": "Type code or selector used by the operation.",
        "physicsType": "Physics shape type code.",
        "shapeId": "Shape identifier.", "bodyIdOrIndex": "Body identifier or body index accepted by the wrapper.",
        "rotateToSign": "Direction sign for rotation toward the target angle.",
        "considerAlreadyColliding": "Whether already-colliding bodies are included in the query.",
        "allowed": "Whether the requested operation is allowed.",
        "isSensor": "Whether the shape or fixture is a sensor.",
        "invDt": "Inverse time step in seconds.",
        "sd": "Source or shape data used by this operation.",
        "insetVar": "Inset value or object used when building the shape.",
        "handle_or_texture": "Texture handle or Texture object.",
        "a_or_b": "First or second operand, depending on the overload.",
        "p0_or_typeName": "First argument or type name, depending on the overload.",
        "px_or_position": "X coordinate or Vector2D position, depending on the overload.",
        "arg0_or_arg1": "First argument in the selected overload.",
        "colourArg": "Colour object or packed colour value.",
        "alphaArg": "Alpha opacity value.",
        "def_or_typeArg": "Shape definition table or type selector, depending on the overload.",
        "unixText": "Text formatted as a Unix timestamp or date.",
        "serverText": "Text returned by the server.",
        "newChar": "Character inserted into the string.",
        "needle": "Substring to search for.", "haystack": "String searched for the substring.",
        "uidF": "Entity unique identifier.", "dlcPack": "DLC pack identifier.",
        "cleanupPrevious": "Whether to release the previous value before replacing it.",
        "index1Based": "One-based index of the item.",
        "visibilityAabbScaleX": "Horizontal scale applied to the visibility bounds.",
        "visibilityAabbScaleY": "Vertical scale applied to the visibility bounds.",
        "g_currentColourSchemeID": "Current colour-scheme identifier.",
        "unixText": "Unix timestamp string.",
        "serverText": "Server-provided text value.",
        "url": "URL or network resource address.",
        "msg": "Message text.", "json": "JSON text to parse.",
        "encoded": "Encoded data to decode.", "selector": "Selector identifying the requested item.",
        "tokens": "Token collection used by the parser.", "uid": "Entity unique identifier.",
        "srcVar": "Source value to copy or convert.", "tableVar": "Table supplied to the operation.",
        "procVar": "Process handle or process table.",
        "newChar": "Character value to insert.", "rx": "Horizontal radius.", "ry": "Vertical radius.",
        "lp": "Low-pass filter parameter.", "sbv": "Scalar value used by the current binding.",
    }
    descriptions = {
        re.sub(r"[^a-z]", "", key.casefold()): value
        for key, value in descriptions.items()
    }
    if key == "b":
        if "colour" in method.casefold() or "color" in method.casefold() or "palette" in method.casefold():
            return "Blue colour component."
        return "Second value or operand."
    if key in ("r", "g") and not any(token in method.casefold()
                                       for token in ("colour", "color", "palette")):
        return "Value supplied to this method."
    if key in ("t", "perc") and "interpol" in method.casefold():
        return "Interpolation amount, clamped to the range 0–1 by the wrapper."
    if key in descriptions:
        return descriptions[key]
    tokens = WORDS.findall(method)
    if tokens and tokens[0].casefold() == "set" and key in ("v", "value", "arg1"):
        target = " ".join(word.lower() for word in tokens[1:])
        return f"New {target or 'property'} value."
    if key in ("v", "value"):
        return "Value supplied to the method; the method name identifies the value being changed or processed."
    if re.fullmatch(r"arg\d+", name):
        return "Argument read at this position; its specific meaning has not been recovered."
    words = human_words(name)
    if words and not re.fullmatch(r"arg\d+", name):
        return f"{words[0].upper() + words[1:]} value supplied to this operation."
    return f"Argument {name} is read by the wrapper; its meaning is not recovered."


def parse_parameters(body: str):
    found = {}
    for macro, arg, index, default in PARAM_MACRO.findall(body):
        if macro.startswith("GM_CHECK_") and macro not in PARAM_TYPES:
            continue
        try:
            position = int(index)
        except ValueError:
            position = None
        found[arg] = {
            "index": position,
            "type": PARAM_TYPES.get(macro, "value"),
            "default": (default or "").strip(),
        }
    for checked, object_type, type_symbol, arg, index in USER_PARAM_MACRO.findall(body):
        position = int(index)
        found[arg] = {
            "index": position,
            "type": script_type_name(type_symbol),
            "default": "",
        }
    for ctype, arg, param_kind, index in PARAM_ASSIGNMENT.findall(body):
        position = int(index)
        inferred_type = {
            "auto": {"FloatOrInt": "number", "Int": "int", "Float": "number",
                     "String": "string", "Table": "table"}.get(param_kind, "value"),
            "gmint": "int", "int": "int", "unsigned int": "int",
            "gmfloat": "number", "float": "number", "double": "number",
            "bool": "bool", "const char *": "string",
            "gmTableObject *": "table", "gmVariable": "value",
        }.get(ctype.replace("  ", " "), {
            "FloatOrInt": "number", "Int": "int", "Float": "number",
            "String": "string", "Table": "table",
        }.get(param_kind, "value"))
        found.setdefault(arg, {"index": position, "type": inferred_type, "default": ""})
    for ctype, arg, param_kind, index in PARAM_CONTEXT_ASSIGNMENT.findall(body):
        position = int(index)
        inferred_type = {
            "gmVariable": "value", "gmTableObject *": "table", "gmint": "int",
            "gmfloat": "number", "std::string": "string", "unsigned int": "int",
            "int": "int", "float": "number", "double": "number", "bool": "bool",
            "auto": {"FloatOrInt": "number", "Int": "int", "Float": "number",
                     "String": "string", "Table": "table"}.get(param_kind, "value"),
        }.get(ctype.replace("  ", " "), {
            "FloatOrInt": "number", "Int": "int", "Float": "number",
            "String": "string", "Table": "table",
        }.get(param_kind, "value"))
        found.setdefault(arg, {"index": position, "type": inferred_type, "default": ""})
    for match in re.finditer(
            r"(?:a_thread|thread)->Param(FloatOrInt|Int|Float|String|Table)?\s*\(\s*(\d+)"
            r"(?:\s*,\s*([^)]*))?\)", body):
        kind, index, default = match.groups()
        arg = f"arg{int(index) + 1}"
        found.setdefault(arg, {
            "index": int(index), "type": {"FloatOrInt": "number", "Int": "int",
                "Float": "number", "String": "string", "Table": "table"}.get(kind, "value"),
            "default": (default or "").strip(),
        })
    for match in re.finditer(
            r"GetVector2D\([^;]*?Param\s*\(\s*(\d+)\s*\)\s*,\s*"
            r"([A-Za-z_]\w*)\s*,\s*([A-Za-z_]\w*)", body):
        index, x_name, y_name = match.groups()
        name = {
            ("sx", "sy"): "start", ("ex", "ey"): "end",
            ("fx", "fy"): "force", ("ax", "ay"): "anchorA",
            ("bx", "by"): "anchorB", ("x0", "y0"): "point1",
            ("x1", "y1"): "point2", ("x", "y"): "position",
        }.get((x_name, y_name), f"vector{int(index) + 1}")
        found.setdefault(name, {"index": int(index), "type": "Vector2D", "default": ""})
    for match in re.finditer(r"GetB2BodyFromVar\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("body1" if index == 0 else "body2", {
            "index": index, "type": "PhysicsBody", "default": ""})
    for match in re.finditer(r"SequenceTable\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("source" if index == 0 else "destination", {
            "index": index, "type": "table", "default": ""})
    for match in re.finditer(r"AsShapeObject\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("reuseShape", {"index": index, "type": "Shape or null", "default": ""})
    for match in re.finditer(r"(?:ResolveTextureHandle|GetTextureShaderType|ResolveTextureResourceLease)\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("texture", {"index": index, "type": "Texture", "default": ""})
    for match in re.finditer(r"GetShaderStorageId\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("shader", {"index": index, "type": "Shader", "default": ""})
    for match in re.finditer(r"(?:ResolveColour|ResolveAnimColour)\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("colour", {"index": index, "type": "Colour", "default": ""})
    for match in re.finditer(r"ReadExcludeList\([^;]*?Param\s*\(\s*(\d+)\s*\)", body):
        index = int(match.group(1))
        found.setdefault("excludeBodies", {"index": index, "type": "array or table", "default": ""})
    if "ReadVec2Args(" in body:
        found.setdefault("vector_or_x", {"index": 0, "type": "Vector2D or number", "default": ""})
        found.setdefault("y", {"index": 1, "type": "number", "default": "",
                                "optional": "when the first argument is a Vector2"})
    for match in re.finditer(r"ReadVec2\s*\(\s*a_thread\s*,\s*(\d+)", body):
        index = int(match.group(1))
        found.setdefault(f"vector{index + 1}", {
            "index": index, "type": "Vector2D", "default": ""})
    if "ReadSingleChar(" in body:
        found.setdefault("character", {"index": 0, "type": "int or string", "default": ""})
    # A position can have different meanings in supported overloads. Keep
    # those names together and discard the generic argN placeholder.
    grouped = defaultdict(list)
    for arg, info in found.items():
        grouped[info["index"]].append((arg, info))
    normalized = {}
    for index, entries in grouped.items():
        named = [(arg, info) for arg, info in entries
                 if not re.fullmatch(r"arg\d+", arg)]
        entries = named or entries
        aliases = list(dict.fromkeys(arg for arg, _ in entries))
        if len(aliases) == 1:
            normalized[aliases[0]] = entries[0][1]
            continue
        info = dict(entries[0][1])
        info["type"] = " or ".join(dict.fromkeys(value["type"] for _, value in entries))
        info["default"] = next((value["default"] for _, value in entries if value["default"]), "")
        normalized["_or_".join(aliases)] = info
    found = normalized
    count = CHECK_PARAMS.search(body)
    if count:
        minimum = int(count.group(1))
        for index in range(int(count.group(1))):
            if not any(info["index"] == index for info in found.values()):
                found[f"arg{index + 1}"] = {
                    "index": index, "type": "value", "default": ""}
        for info in found.values():
            index = info["index"]
            if index is None:
                continue
            if index < minimum:
                info["default"] = ""
            elif info.get("default"):
                info["optional"] = "when omitted"
    return sorted(found.items(), key=lambda item: (
        item[1]["index"] is None, item[1]["index"] or 0, item[0]))


def return_types(name: str, body: str):
    types = []
    boolean_name = bool(re.search(r"(?:^|_)(?:is|has|can)(?=[A-Z_])|(?:^|_)getIs",
                                  name, re.IGNORECASE))
    for _, push in PUSH_CALL.findall(body):
        kind = PUSH_TYPES.get(push, "value")
        if push == "PushNewUser":
            type_match = re.search(
                r"PushNewUser\s*\([^;]*?,\s*(g_[A-Za-z_]\w*Type)\s*\)",
                body, re.DOTALL)
            if type_match:
                kind = script_type_name(type_match.group(1))
        if push == "PushInt" and boolean_name:
            kind = "bool"
        if push == "Push":
            pushed = re.search(r"->Push\s*\(\s*([A-Za-z_]\w*)", body)
            constructor = pushed.group(1) if pushed else ""
            if constructor == "MakeVector2D":
                kind = "Vector2D"
            elif constructor.startswith("MakeColour") or constructor.startswith("MakeColor"):
                kind = "Colour"
        if kind not in types:
            types.append(kind)
    return types


def page_id(family: str, owner: str, name: str):
    key = f"{family}:{owner}:{name}"
    return "api_function_" + sha1(key.encode("utf-8")).hexdigest()[:10]


def reference_page_id(prefix: str, owner: str, name: str):
    key = f"{prefix}:{owner}:{name}"
    return f"api_{prefix}_" + sha1(key.encode("utf-8")).hexdigest()[:10]


def script_type_name(type_symbol: str):
    name = re.sub(r"^g_", "", type_symbol)
    name = re.sub(r"Type$", "", name)
    return name[:1].upper() + name[1:]


def signature(name: str, owner: str, params, result: str):
    arguments = []
    for param, info in params:
        optional = "?" if info.get("optional") and not info.get("default") else ""
        default = f" = {info['default']}" if info.get("default") else ""
        arguments.append(f"{param}{optional}: {info['type']}{default}")
    prefix = f"{owner}." if owner != "Global" else ""
    return f"{prefix}{name}({', '.join(arguments)}) -> {result}"


def load_function_notes():
    path = ROOT / "docs/api/function_notes.tsv"
    if not path.exists():
        return {}
    with path.open(encoding="utf-8") as handle:
        rows = csv.DictReader(handle, delimiter="\t")
        return {row["name"]: {key: ("" if value == "-" else value)
                              for key, value in row.items()}
                for row in rows}


def main():
    families = defaultdict(lambda: defaultdict(lambda: defaultdict(list)))
    sources = {path: path.read_text(encoding="utf-8", errors="replace")
               for path in sorted(SCRIPT_API_SOURCES)}
    handlers = {}
    custom_type_map = {}
    custom_type_names = {}
    script_libraries = []
    type_aliases = {
        alias: target
        for source in sources.values()
        for alias, target in CUSTOM_TYPE_ALIAS.findall(source)
    }

    def canonical_type(type_symbol):
        seen = set()
        while type_symbol in type_aliases and type_symbol not in seen:
            seen.add(type_symbol)
            type_symbol = type_aliases[type_symbol]
        return type_symbol

    for path, source in sources.items():
        registered_types = {
            canonical_type(type_symbol): table
            for type_symbol, table in TYPE_REGISTRATION.findall(source)
        }
        for type_symbol in CUSTOM_TYPE_SYMBOL.findall(source):
            canonical = canonical_type(type_symbol)
            custom_type_map.setdefault(canonical,
                (path, registered_types.get(canonical, ""), script_type_name(canonical)))
        for type_symbol, script_type in CUSTOM_TYPE_CREATE.findall(source):
            custom_type_names[canonical_type(type_symbol)] = script_type
        script_libraries.extend((path, table, name)
                                for table, name in LIBRARY_REGISTRATION.findall(source))
        for match in HANDLER_DEF.finditer(source):
            handler = match.group(1)
            if handler not in handlers:
                body, line, description = body_for(source, handler)
                handlers[handler] = (path, body, line, description)
    for path, source in sources.items():
        registrations = []
        for table, name, handler in table_entries(source):
            owner = owner_name(table, source)
            # script_compat_bindings.cpp builds two separate local arrays
            # named `lib` at runtime. The handler prefixes identify which
            # GameMonkeyScript user type owns each entry.
            if table == "lib" and name == "setContour" and handler == "gmf_alarm_noop":
                owner = "LFO"
            elif table == "lib" and handler.startswith("gmf_alarm_"):
                owner = "ALARM"
            elif table == "lib" and handler.startswith("gmf_lfo_"):
                owner = "LFO"
            registrations.append((owner, name, handler))
        for name, handler, owner in DIRECT_ENTRY.findall(source):
            registrations.append((owner or "Global", name, handler))
        for name, handler in GLOBAL_FUNCTION_REGISTRATION.findall(source):
            registrations.append(("Global", name, handler))
        if path.name == "entity_bindings.cpp":
            inject = re.search(
                r'void\s+InjectEntityHandleMethods\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*bool\s+TryAutoLoadEntityScript',
                source, re.DOTALL)
            if inject:
                registrations.extend(
                    ("Entity", name, handler)
                    for name, handler in ENTITY_INSTANCE_REGISTRATION.findall(inject.group(1)))
        if path.name == "process_handle_bindings.cpp":
            inject = re.search(
                r'void\s+InjectProcessHandleMethods\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*void\s+RegisterProcessHandleBindings',
                source, re.DOTALL)
            if inject:
                body = inject.group(1)
                registrations.extend(
                    ("Process", name, handler)
                    for name, handler in SET_NATIVE_IF_MISSING.findall(body))
                registrations.extend(
                    ("Process", name, handler)
                    for name, handler in TABLE_FUNCTION_REGISTRATION.findall(body))
        if path.name == "video_bindings.cpp":
            inject = re.search(
                r'void\s+InjectProcessBaseMethods\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*void\s+RegisterVideoBindings', source, re.DOTALL)
            if inject:
                registrations.extend(
                    ("Process", name, handler)
                    for name, handler in TABLE_FUNCTION_REGISTRATION.findall(inject.group(1)))
        if path.name == "entity_bindings.cpp":
            player = re.search(
                r'void\s+InjectPlayerRendererLifecycleMethods\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}', source, re.DOTALL)
            if player:
                registrations.extend(
                    ("Player", name, handler)
                    for name, handler in TABLE_FUNCTION_REGISTRATION.findall(player.group(1)))
            registrations.extend([
                ("Entity", "giveBackToBulletCache", "gmf_entity_delegate_giveBackToBulletCache"),
                ("Entity", "onMusicTrigger", "gmf_entity_delegate_onMusicTrigger"),
                ("Entity", "configure", "gmf_entity_configureTrampoline"),
                ("Player", "testfn", "gmf_player_testfn"),
            ])
        if path.name == "gamesystem_bindings.cpp":
            # runProcess installs this wrapper as configure on one popup type.
            if "gmf_popupProcessConfigureOfflineCompat" in source:
                registrations.append(("Process", "configure", "gmf_popupProcessConfigureOfflineCompat"))
            offline = re.search(
                r'gmVariable\s+MakeOfflineCommunityProcess\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*int\s+GM_CDECL', source, re.DOTALL)
            if offline:
                registrations.append(("OfflineCommunityProcess", "isInited", "OfflineCommunityProcessIsInited"))
                noop_array = re.search(
                    r'kNoOpMethods\[\]\s*=\s*\{(.*?)\}', offline.group(1), re.DOTALL)
                if noop_array:
                    registrations.extend(("OfflineCommunityProcess", name, "OfflineCommunityProcessNoOp")
                                         for name in re.findall(r'"([^"\n]+)"', noop_array.group(1)))
                registrations.extend([
                    ("OfflineProfileState", "getCurrentUserID", "OfflineCommunityProcessZero"),
                    ("OfflineProfileState", "getCurrentButton", "OfflineCommunityProcessZero"),
                    ("OfflineCommunityState", "getState", "OfflineCommunityProcessMinusOne"),
                    ("OfflineCommunityState", "changeState", "OfflineCommunityProcessNoOp"),
                    ("OfflineLoginError", "execute", "OfflineLoginErrorNoOp"),
                ])
        if path.name == "audio_bindings.cpp":
            effect = re.search(
                r'gmVariable\s+MakeEffectTable\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*int\s+GM_CDECL\s+gmf_chain_addEffect',
                source, re.DOTALL)
            if effect:
                for method in ("getType", "setBypass", "setParam"):
                    entry = re.search(
                        rf't->Set\s*\(&a_machine,\s*"{method}"\s*,\s*'
                        r'gmVariable\s*\(\s*a_machine\.AllocFunctionObject\s*\(\s*\n\s*\[\]',
                        effect.group(1))
                    if entry:
                        handler = "inline_SoundEffect_" + method
                        registrations.append(("SoundEffect", method, handler))
                        handlers[handler] = (path, "", source.count("\n", 0,
                            effect.start(1) + entry.start()) + 1, "")
            chain = re.search(
                r'gmVariable\s+MakeEffectChainTable\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*gmVariable\s+GetOrCreateMasterEffectChain',
                source, re.DOTALL)
            if chain:
                registrations.extend(
                    ("EffectChain", name, handler)
                    for name, handler in TABLE_FUNCTION_REGISTRATION.findall(chain.group(1)))
            voices = re.search(
                r'int\s+GM_CDECL\s+gmf_getAllVoices\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*int\s+GM_CDECL', source, re.DOTALL)
            if voices:
                for method in ("isValid", "get", "goToNext"):
                    entry = re.search(rf'it->Set\s*\(&m,\s*"{method}"\s*,\s*'
                                      r'gmVariable\s*\(\s*m\.AllocFunctionObject\s*\(\s*\n\s*\[\]',
                                      voices.group(1))
                    if entry:
                        handler = "inline_VoiceIterator_" + method
                        registrations.append(("VoiceIterator", method, handler))
                        handlers[handler] = (path, "", source.count("\n", 0,
                            voices.start(1) + entry.start()) + 1, "")
            audio_ticket = re.search(
                r'ticket->Set\s*\(machine,\s*"getSize"\s*,\s*'
                r'gmVariable\s*\(machine->AllocFunctionObject\s*\(\s*'
                r'(gmf_audioSaveRequestGetSize)\s*\)\s*\)\s*\)', source, re.DOTALL)
            if audio_ticket:
                registrations.append(("AudioSaveRequest", "getSize", audio_ticket.group(1)))
        if path.name == "savedata_bindings.cpp":
            ticket = re.search(
                r'gmVariable\s+MakeSizeTicket\s*\([^)]*\)\s*\{'
                r'(.*?)\n\}\s*\n\s*int\s+GM_CDECL\s+gmf_SAVEDATA_reqGetFileSize',
                source, re.DOTALL)
            if ticket:
                value_handlers = {
                    "g_getSizeFn": "gmf_ticket_getSize",
                    "g_getFilenameFn": "gmf_ticket_getFilename",
                    "g_getIfDoneFn": "gmf_ticket_getIfDone",
                    "g_getIfErrorFn": "gmf_ticket_getIfError",
                }
                registrations.extend(
                    ("SaveDataTicket", name, value_handlers[function])
                    for name, function in TABLE_VALUE_FUNCTION_REGISTRATION.findall(ticket.group(1)))
        for owner, name, handler in registrations:
            handler_path, body, line, description = handlers.get(handler, (path, "", 0, ""))
            families[path.stem][owner][name].append((handler_path, handler, body, line, description))
    function_notes = load_function_notes()

    target = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "docs/api/_generated"
    target.mkdir(parents=True, exist_ok=True)
    for old_file in target.glob("*.dox"):
        old_file.unlink()

    index = ["/**", "\\page script_api_index GameMonkeyScript API Reference", "",
             "\\warning EARLY AND INCOMPLETE REFERENCE. OpenEgg is experimental. The missing-method list is limited to retail entries recovered so far; it is not a complete retail API inventory.", "",
             "Every registered function or method has its own reference page with a description, parameters,",
             "return information, and verification state. Descriptions without source comments are summarized",
             "from the binding name. Argument types and defaults are taken from the wrapper where available.", "",
             "- **Registration confirmed**: the name is registered here.",
             "- **OpenEgg runtime not verified**: no per-function runtime result is recorded.",
             "- **Retail match not verified**: no per-function retail comparison is recorded.", "",
             "- **Not implemented**: retail method name was recovered, but OpenEgg has no registration for it.", "",
             "\\section script_types Libraries and custom script types", "",
             "Libraries:", ""]
    library_pages = []
    seen_libraries = set()
    for path, table, display_name in script_libraries:
        if display_name in seen_libraries:
            continue
        seen_libraries.add(display_name)
        library_page = reference_page_id("library", display_name, display_name)
        library_pages.append((path, table, display_name, library_page))
        index.append(f"- \\ref {library_page} `{display_name}`")
    index += ["", "Native user types:", ""]
    type_pages = []
    for type_symbol, (path, table, display_name) in sorted(custom_type_map.items()):
        type_page = reference_page_id("type", display_name, type_symbol)
        type_pages.append((path, type_symbol, table, display_name, type_page))
        index.append(f"- \\ref {type_page} `{display_name}`")
    manual_types_path = ROOT / "docs/api/custom_types.tsv"
    if manual_types_path.exists():
        with manual_types_path.open(encoding="utf-8") as handle:
            manual_types = list(csv.DictReader(handle, delimiter="\t"))
        if manual_types:
            index += ["", "OpenEgg-specific custom values:", ""]
            for item in manual_types:
                index.append(f"- \\ref {item['page']} `{item['type']}` — {item['description']}")
    index += ["", "\\section script_retail_types Retail native type inventory", "",
              "All 52 type names and method counts below come from recovered retail registration data.",
              "Names absent from OpenEgg are listed with their unrecovered method signatures.", ""]
    retail_types_path = ROOT / "docs/api/retail_types.tsv"
    with retail_types_path.open(encoding="utf-8") as handle:
        retail_types = list(csv.DictReader(handle, delimiter="\t"))
    local_type_pages = {
        custom_type_names[symbol].casefold(): (display_name, page)
        for _, symbol, _, display_name, page in type_pages if symbol in custom_type_names
    }
    local_library_pages = {display_name.casefold(): (display_name, page)
                           for _, _, display_name, page in library_pages}
    retail_type_pages = []
    for item in retail_types:
        name = item["name"]
        local_type = local_type_pages.get(name.casefold())
        local_library = local_library_pages.get(name.casefold())
        if local_type:
            index.append(f"- \\ref {local_type[1]} `{name}` — OpenEgg type page: `{local_type[0]}`")
        elif local_library:
            index.append(f"- \\ref {local_library[1]} `{name}` — exposed locally as a library")
        else:
            page = reference_page_id("retail_type", name, name)
            retail_type_pages.append((item, page))
            index.append(f"- \\ref {page} `{name}` — **not registered locally**")
    index += ["", "\\section retail_missing_functions Retail methods not implemented", "",
             "The following recovered retail methods do not have an OpenEgg binding. Signatures are marked as unrecovered where evidence is not available.", ""]
    missing_path = ROOT / "docs/api/retail_missing.tsv"
    missing = []
    if missing_path.exists():
        with missing_path.open(encoding="utf-8") as handle:
            missing = list(csv.DictReader(handle, delimiter="\t"))
    for item in missing:
        owner, name = item["type"], item["method"]
        page = page_id("retail_missing", owner, name)
        index.append(f"- \\ref {page} `{owner}.{name}` — **Not implemented**")
    index += ["", "Categories:", ""]
    page_count = 0
    for family in sorted(families, key=str.casefold):
        owners = families[family]
        family_page = f"api_{family}"
        index.append(f"- \\ref {family_page} ({sum(len(names) for names in owners.values())} entries)")
        category = ["/**", f"\\page {family_page} {family.replace('_', ' ')}", "",
                    f"Functions and methods registered from `{family}.cpp`.", ""]
        for owner in sorted(owners, key=str.casefold):
            category += [f"\\section {family}_{re.sub('[^A-Za-z0-9]', '_', owner)} {owner}", ""]
            for name in sorted(owners[owner], key=str.casefold):
                category.append(f"- \\ref {page_id(family, owner, name)} `{name}`")
                page_count += len(owners[owner][name])
        category.append("*/")
        (target / f"{family_page}.dox").write_text("\n".join(category), encoding="utf-8")

    unique_pages = 0
    for family in sorted(families, key=str.casefold):
        for owner in sorted(families[family], key=str.casefold):
            for name in sorted(families[family][owner], key=str.casefold):
                records = families[family][owner][name]
                page = page_id(family, owner, name)
                notes = function_notes.get(f"{owner}.{name}", {})
                out = ["/**", f"\\page {page} {name}", "",
                       f"**{owner} method.** Registration confirmed. OpenEgg runtime not verified. Retail match not verified.", "",
                       f"\\section {page}_description Description", ""]
                descriptions = list(dict.fromkeys(record[4] for record in records if record[4]))
                out.extend([notes["description"]] if notes.get("description") else
                           (descriptions or [inferred_description(name, records[0][1])]))
                out += ["", f"\\section {page}_parameters Parameters", ""]
                params = {}
                for _, _, body, _, _ in records:
                    for param, info in parse_parameters(body):
                        key = info["index"] if info["index"] is not None else param
                        params.setdefault(key, (param, info))
                if notes.get("parameters_json"):
                    authored_params = json.loads(notes["parameters_json"])
                    params = {item["index"]: (item["name"], item)
                              for item in authored_params}
                if params:
                    for key in sorted(params, key=lambda item: (isinstance(item, str), item)):
                        param, info = params[key]
                        default = f"; default `{info['default']}`" if info.get("default") else ""
                        optional = f"; optional {info['optional']}" if info.get("optional") else ""
                        parameter_notes = json.loads(notes.get("parameter_notes", "{}") or "{}")
                        param_note = info.get("description") or parameter_notes.get(param)
                        if not param_note and "_or_" in param:
                            param_note = parameter_notes.get(param.split("_or_")[0])
                        out.append(f"- **`{param}`** (`{info['type']}`{default}{optional}): {param_note or parameter_description(param, name)}")
                checks = list(dict.fromkeys(
                    match.group(1) for _, _, body, _, _ in records
                    if (match := CHECK_PARAMS.search(body))))
                if checks:
                    out += ["", "Minimum required argument count: " + ", ".join(f"`{n}`" for n in checks) + "."]
                if not params:
                    bodies = [body for _, _, body, _, _ in records]
                    reads_params = any(re.search(r"\b(?:Param|Read[A-Za-z0-9_]*Args?)\s*\(", body)
                                       for body in bodies)
                    checks_count = any("GetNumParams" in body or "GM_NUM_PARAMS" in body
                                       for body in bodies)
                    if notes.get("parameters_json") == "[]" or (not reads_params and not checks_count):
                        out.append("None.")
                    else:
                        out.append("The wrapper inspects arguments dynamically; parameter names and types are not fully recovered.")
                if notes.get("parameters_note"):
                    out += ["", "**Note:** " + notes["parameters_note"]]
                possible_types = list(dict.fromkeys(
                    kind for _, _, body, _, _ in records for kind in return_types(name, body)))
                signature_params = [params[key] for key in sorted(
                    params, key=lambda item: (isinstance(item, str), item))]
                result_type = notes.get("return_type") or (" or ".join(possible_types) if possible_types else "no value pushed")
                out += ["", f"\\section {page}_signature Signature", "",
                        "`" + (notes.get("signature") or signature(name, owner, signature_params, result_type)) + "`", ""]
                out += ["", f"\\section {page}_returns Returns", ""]
                documented_types = ([notes["return_type"]]
                                    if notes.get("return_type") and notes["return_type"] != "no value pushed"
                                    else possible_types)
                if documented_types:
                    out.append("Possible script value types: " + ", ".join(f"`{value}`" for value in documented_types) + ".")
                elif notes.get("return_type"):
                    if notes["return_type"] == "no value pushed":
                        out.append("No script value is pushed by the native wrapper.")
                    else:
                        out.append(f"Returns `{notes['return_type']}`.")
                else:
                    out.append("No script value is pushed by the native wrapper.")
                if notes.get("returns_note"):
                    out.append(notes["returns_note"])
                out += ["", f"\\section {page}_implementation Implementation", ""]
                for path, handler, _, line, _ in records:
                    relative = path.relative_to(ROOT).as_posix()
                    out.append(f"- `{handler}` — `{relative}:{line or 1}`")
                out += ["", "This documents the OpenEgg wrapper. Verify runtime behavior and retail compatibility independently.", "", "*/"]
                (target / f"{page}.dox").write_text("\n".join(out), encoding="utf-8")
                unique_pages += 1

    type_descriptions = {
        "stringstream": "Builds a string by appending formatted values and text.",
        "videodrawbuffer": "Stores draw commands for later submission to the video renderer.",
        "sequencer": "Manages sequencer state, triggers, and actions for timed audio playback.",
        "sequencerlistref": "A reference to a list of sequencers.",
        "sequencertrigger": "A trigger entry used by a sequencer.",
        "sequencertriggerlist": "A reference to a sequencer's trigger list.",
        "sequenceraction": "A script-visible action backed by a mutable GameMonkeyScript table.",
        "vector2d": "A two-dimensional vector with mutable x and y components.",
        "processlistref": "A reference to a list of game processes.",
        "colour": "An RGB colour whose channels are stored as normalized values.",
        "physicsbody": "A handle to a physics body and its script-visible properties.",
        "physicsdata": "Physics body data used by collision and entity operations.",
        "physicsshape": "A handle to a shape registered with the physics system.",
        "collisioninfo": "Information about a collision between physics bodies.",
        "shapecollisioninfo": "Collision information associated with a particular shape.",
        "intiter": "An iterator over a native integer list.",
        "shader": "A handle to a script-visible shader resource.",
        "alarmhandle": "A handle used to identify a scheduled alarm.",
        "lfo": "A low-frequency oscillator used by script-controlled modulation.",
        "envelope": "An envelope describing a value over time.",
        "mixerparams": "Mixer settings associated with an entity or instrument.",
        "telescope": "A script-visible handle for a camera-follow target.",
        "entitylistiter": "An iterator over entities in an entity list.",
        "instrument": "An instrument configuration used by the audio sequencer.",
        "physicsshapedef": "A collection of definitions used to create physics shapes.",
        "matrix4": "A four-by-four transform matrix used by script-facing rendering operations.",
        "file": "A script-visible file handle for reading and writing user-accessible files.",
        "trianglelfo": "A triangle-wave low-frequency oscillator.",
        "anim": "A handle to an animation resource and its playback state.",
        "shape": "A mutable shape built from vertices and optional texture data.",
        "shapevertex": "A reference to one shape vertex, including position and texture coordinates.",
        "rayiter": "An iterator over entities returned by a physics ray query.",
        "joint": "A handle to a physics joint.",
        "jointiter": "An iterator over physics joints.",
        "texture": "A handle to a script-visible texture resource.",
        "pulseperc": "A pulse percussion generator with attack and decay settings.",
    }
    for path, type_symbol, table, display_name, type_page in type_pages:
        owner = owner_name(table, sources[path]) if table else ""
        if type_symbol == "g_alarmHandleType":
            owner = "ALARM"
        elif type_symbol == "g_lfoType":
            owner = "LFO"
        methods = []
        if owner:
            for family in families.values():
                methods.extend((family_owner, name) for family_owner, names in family.items()
                               if family_owner == owner for name in names)
        description = type_descriptions.get(
            display_name.casefold(),
            f"A native GameMonkeyScript user type named `{display_name}`.")
        type_doc = ["/**", f"\\page {type_page} {display_name}", "",
                    "\\section " + type_page + "_description Description", "", description, "",
                    "\\section " + type_page + "_properties Properties", ""]
        properties = {
            "vector2d": ["`x` (`number`): Horizontal component.", "`y` (`number`): Vertical component."],
            "shapevertex": ["`x` (`number`): Horizontal vertex position.", "`y` (`number`): Vertical vertex position.", "`u` (`number`): Horizontal texture coordinate.", "`v` (`number`): Vertical texture coordinate."],
            "sequenceraction": ["Dynamic keys are read from and written to the action's backing table."],
        }.get(display_name.casefold(), [])
        type_doc += (properties or ["No public properties have been recovered for this type."]) + ["",
                    "\\section " + type_page + "_construction Construction", "",
                    "This is registered by OpenEgg as a native GameMonkeyScript user type.",
                    (f"Its GameMonkeyScript type name is `{custom_type_names[type_symbol]}`."
                     if type_symbol in custom_type_names else ""),
                    "Constructor and factory functions are listed below when their return type is known.", "",
                    "\\section " + type_page + "_constructors Constructors", ""]
        constructors = []
        script_type_names = {display_name.casefold()}
        if type_symbol in custom_type_names:
            script_type_names.add(custom_type_names[type_symbol].casefold())
        for family_name, groups in families.items():
            for method_owner, names in groups.items():
                for method_name, records in names.items():
                    result_names = [result for _, _, body, _, _ in records
                                    for result in return_types(method_name, body)]
                    note_result = function_notes.get(f"{method_owner}.{method_name}", {}).get("return_type", "")
                    result_names.extend(part.strip() for part in note_result.split(" or "))
                    if any(result.casefold() in script_type_names for result in result_names):
                        constructors.append((family_name, method_owner, method_name))
        if constructors:
            for family_name, method_owner, method_name in sorted(set(constructors)):
                type_doc.append(f"- \\ref {page_id(family_name, method_owner, method_name)} `{method_name}`")
        else:
            type_doc.append("No constructor or factory return type was recovered.")
        type_doc += ["",
                    "\\section " + type_page + "_methods Methods", ""]
        if methods:
            for method_owner, method in sorted(set(methods), key=lambda item: item[1].casefold()):
                # A method page can occur in only one source family for a given owner/name pair.
                family_name = next(f for f, groups in families.items()
                                   if method_owner in groups and method in groups[method_owner])
                type_doc.append(f"- \\ref {page_id(family_name, method_owner, method)} `{method}`")
        else:
            type_doc.append("No method table entries were associated with this type.")
        type_doc += ["", "Registered by OpenEgg from the binding source. Retail behavior is not implied by the local type registration.", "", "*/"]
        (target / f"{type_page}.dox").write_text("\n".join(type_doc), encoding="utf-8")

    for item, type_page in retail_type_pages:
        name = item["name"]
        own_methods = item["own_methods"]
        base_methods = item["base_methods"]
        methods = f"{own_methods} own method entries"
        if base_methods != "-":
            methods += f" and {base_methods} inherited method entries"
        type_doc = [
            "/**", f"\\page {type_page} {name}", "",
            "\\section " + type_page + "_description Description", "",
            f"`{name}` is a native GameMonkeyScript type registered by the retail engine.", "",
            "\\section " + type_page + "_status OpenEgg status", "",
            "OpenEgg does not register a native user type with this name.", "",
            "\\section " + type_page + "_methods Methods", "",
            f"The recovered retail type table records {methods}.",
            "Method names, parameters, and return values are not yet recovered in this reference.", "",
            "This page documents the retail type inventory only; it does not imply OpenEgg compatibility.", "*/"
        ]
        (target / f"{type_page}.dox").write_text("\n".join(type_doc), encoding="utf-8")

    library_descriptions = {
        "audio": "Controls sound playback, voices, volumes, and recorded audio scripts.",
        "video": "Provides rendering, display, camera, and draw-list operations.",
        "gamesystem": "Controls game processes, simulation timing, and shared runtime state.",
        "input": "Reads controller, keyboard, mouse, and touch input state.",
        "system": "Provides system and file operations exposed to scripts.",
        "colourmanager": "Loads and manages named colours and colour palettes.",
    }
    for path, table, display_name, library_page in library_pages:
        methods = []
        for family_name, groups in families.items():
            if display_name in groups:
                methods.extend((family_name, name) for name in groups[display_name])
        description = library_descriptions.get(
            display_name.casefold(),
            f"Provides the `{display_name}` script library registered by OpenEgg.")
        library_doc = ["/**", f"\\page {library_page} {display_name}", "",
                       "\\section " + library_page + "_description Description", "", description, "",
                       "\\section " + library_page + "_methods Methods", ""]
        if methods:
            for family_name, method in sorted(set(methods), key=lambda item: item[1].casefold()):
                family_owner = display_name
                library_doc.append(f"- \\ref {page_id(family_name, family_owner, method)} `{method}`")
        else:
            library_doc.append("No registered methods were associated with this library.")
        library_doc += ["", "Methods are documented from OpenEgg's binding registrations. Retail behavior is not implied by registration.", "", "*/"]
        (target / f"{library_page}.dox").write_text("\n".join(library_doc), encoding="utf-8")

    for item in missing:
        owner, name = item["type"], item["method"]
        page = page_id("retail_missing", owner, name)
        out = ["/**", f"\\page {page} {owner}.{name}", "",
               "\\warning Not implemented in OpenEgg.", "",
               f"**Retail type:** `{owner}`", "",
               "\\section " + page + "_description Description", "",
               item["description"], "",
               "\\section " + page + "_parameters Parameters", "",
               item["parameters"], "",
               "\\section " + page + "_returns Returns", "",
               item["returns"], "",
               "Treat inferred details as provisional; confirm the complete retail contract before relying on compatibility.", "", "*/"]
        (target / f"{page}.dox").write_text("\n".join(out), encoding="utf-8")

    index += ["", f"{page_count} registration entries; {unique_pages} OpenEgg method pages in {len(families)} categories; {len(missing)} recovered retail methods are not implemented.",
              "Rebuild after changing a registration or handler.", "", "*/"]
    (target / "script_api_index.dox").write_text("\n".join(index), encoding="utf-8")
    print(f"Wrote {page_count} registrations ({unique_pages} method pages) in {len(families)} categories to {target}")


if __name__ == "__main__":
    main()
