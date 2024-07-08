import subprocess

def extract_commit() -> str:
    git_describe_output = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True, check=True).stdout.strip()
    split_output = git_describe_output.split("-")

    if (len(split_output) >= 3):
        return f"{split_output[0]}\\ ({split_output[2][1:]})"
    else:
        return split_output[0]

try:
    version = extract_commit()
except:
    version = "Unknown"

flag = "-D REPO_VERSION=\\\"" + version + "\\\""
dev_flag = "-DREPO_DEVELOPMENT=1"
print(f"Version: {version}")
print(f"Flag: {flag}")

flags = [flag]

if ('(' in version):
    flags.append(dev_flag)

Import("env")

env.Append(
    BUILD_FLAGS=flags
)