import subprocess

def extract_commit() -> tuple[str, str]: # Version, Commit
    git_describe_output = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True, check=True).stdout.strip()
    split_output = git_describe_output.split("-")
    return split_output[0], split_output[2][1:]

try:
    data = extract_commit()
    version = f"{data[0]}\\ ({data[1]})"
except:
    version = "Unknown"


flag = "-D REPO_VERSION=\\\"" + version + "\\\""
print(f"Version: {version}")
print(f"Flag: {flag}")

Import("env")

env.Append(
    BUILD_FLAGS=[flag]
)