from setuptools import setup, find_packages

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="battery-saver-g16",
    version="0.1.0",
    author="Caleb Cho",
    description="One-click battery optimization for ASUS Zephyrus G16",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/CALEBCH0/savemode.git",
    py_modules=["battery_saver"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: Microsoft :: Windows",
        "Development Status :: 3 - Alpha",
        "Intended Audience :: End Users/Desktop",
        "Topic :: System :: Hardware",
        "Topic :: System :: Power (UPS)",
    ],
    python_requires=">=3.8",
    install_requires=[
        "wmi>=1.5.1",
        "pywin32>=306",
    ],
    entry_points={
        "console_scripts": [
            "battery-saver=battery_saver:main",
        ],
    },
)