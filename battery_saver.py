import sys
import os
import subprocess
import ctypes
import json
from pathlib import Path
from enum import Enum
from typing import Dict, List, Optional
import tkinter as tk
from tkinter import ttk, messagebox
import threading
import logging

logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)


class PowerProfile(Enum):
    BATTERY_SAVER = "battery_saver"
    PERFORMANCE = "performance"


class PowerManager:
    def __init__(self):
        self.power_plans = self._get_power_plans()
        
    def _run_powershell(self, command: str) -> str:
        try:
            result = subprocess.run(
                ["powershell", "-Command", command],
                capture_output=True,
                text=True,
                check=True
            )
            return result.stdout.strip()
        except subprocess.CalledProcessError as e:
            logger.error(f"PowerShell command failed: {e}")
            return ""
    
    def _get_power_plans(self) -> Dict[str, str]:
        output = self._run_powershell("powercfg /list")
        plans = {}
        
        # Use standard Windows power scheme GUIDs
        standard_guids = {
            '381b4222-f694-41f0-9685-ff5bb260df2e': 'balanced',
            '8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c': 'performance',
            'a1841308-3541-4fab-bc81-f71556f20b4a': 'saver'
        }
        
        for line in output.split('\n'):
            if 'GUID:' in line:
                for guid, plan_type in standard_guids.items():
                    if guid in line:
                        plans[plan_type] = guid
                        logger.info(f"Found {plan_type} power plan: {guid}")
        
        # Fallback: try to detect by keywords in any language
        if not plans:
            for line in output.split('\n'):
                if 'GUID:' in line:
                    parts = line.split()
                    if len(parts) >= 2:
                        guid = parts[1]
                        # Check for English or Korean keywords
                        if any(keyword in line for keyword in ['Balanced', '균형']):
                            plans['balanced'] = guid
                        elif any(keyword in line for keyword in ['High performance', '고성능']):
                            plans['performance'] = guid
                        elif any(keyword in line for keyword in ['Power saver', '절전']):
                            plans['saver'] = guid
        
        return plans
    
    def set_power_plan(self, profile: PowerProfile):
        # Use built-in Windows scheme aliases instead of GUIDs
        # Note: The naming is counterintuitive!
        # SCHEME_MAX = Power Saver (MAXimum power saving)
        # SCHEME_MIN = High Performance (MINimum power saving)
        # SCHEME_BALANCED = Balanced
        if profile == PowerProfile.BATTERY_SAVER:
            scheme = "SCHEME_MAX"  # Power Saver
        else:
            scheme = "SCHEME_MIN"  # High Performance
        
        result = self._run_powershell(f"powercfg /setactive {scheme}")
        if result or result == "":
            logger.info(f"Set power plan to {profile.value} using {scheme}")
        else:
            # Fallback to GUID method if aliases don't work
            if profile == PowerProfile.BATTERY_SAVER:
                plan_guid = self.power_plans.get('saver', self.power_plans.get('balanced'))
            else:
                plan_guid = self.power_plans.get('performance', self.power_plans.get('balanced'))
            
            if plan_guid:
                self._run_powershell(f"powercfg /setactive {plan_guid}")
                logger.info(f"Set power plan to {profile.value} using GUID")


class DisplayManager:
    def __init__(self):
        self.wmi_available = self._check_wmi()
    
    def _check_wmi(self) -> bool:
        try:
            import wmi
            return True
        except ImportError:
            logger.warning("WMI module not available. Some display features may be limited.")
            return False
    
    def set_brightness(self, level: int):
        level = max(0, min(100, level))
        
        if self.wmi_available:
            try:
                import wmi
                c = wmi.WMI(namespace='wmi')
                methods = c.WmiMonitorBrightnessMethods()[0]
                methods.WmiSetBrightness(level, 0)
                logger.info(f"Set brightness to {level}%")
            except Exception as e:
                logger.error(f"Failed to set brightness via WMI: {e}")
        else:
            try:
                subprocess.run([
                    "powershell", "-Command",
                    f"(Get-WmiObject -Namespace root/WMI -Class WmiMonitorBrightnessMethods).WmiSetBrightness(1,{level})"
                ], check=True)
                logger.info(f"Set brightness to {level}%")
            except Exception as e:
                logger.error(f"Failed to set brightness: {e}")
    
    def set_refresh_rate(self, rate: int):
        try:
            subprocess.run([
                "powershell", "-Command",
                f"$displays = Get-CimInstance -Namespace root\\wmi -ClassName WmiMonitorListedSupportedSourceModes; "
                f"# This would require more complex implementation with display API"
            ])
            logger.info(f"Refresh rate control requires additional display API implementation")
        except Exception as e:
            logger.error(f"Failed to set refresh rate: {e}")


class ProcessManager:
    def __init__(self):
        self.bloatware_processes = [
            "ArmouryCrate.exe",
            "ArmouryCrate.Service.exe",
            "GameBarPresenceWriter.exe",
            "Xbox.TCUI.exe",
            "XboxGameBarSpotify.exe",
            "Discord.exe",
            "DiscordWebHelper.exe",
            "Steam.exe",
            "steamwebhelper.exe",
            "EpicGamesLauncher.exe",
            "Origin.exe",
            "Skype.exe",
            "Teams.exe",
            "slack.exe",
            "spotify.exe"
        ]
    
    def kill_bloatware(self):
        killed = []
        for process in self.bloatware_processes:
            try:
                result = subprocess.run(
                    ["taskkill", "/F", "/IM", process],
                    capture_output=True,
                    text=True
                )
                if result.returncode == 0:
                    killed.append(process)
                    logger.info(f"Killed process: {process}")
            except Exception as e:
                logger.debug(f"Could not kill {process}: {e}")
        
        return killed


class GPUManager:
    def __init__(self):
        self.nvidia_smi_path = self._find_nvidia_smi()
        self.min_power_limit = None
        self.max_power_limit = None
        self.default_power_limit = None
        if self.nvidia_smi_path:
            self._get_power_limits()
    
    def _find_nvidia_smi(self) -> Optional[str]:
        common_paths = [
            r"C:\Program Files\NVIDIA Corporation\NVSMI\nvidia-smi.exe",
            r"C:\Windows\System32\nvidia-smi.exe"
        ]
        
        for path in common_paths:
            if os.path.exists(path):
                return path
        
        try:
            result = subprocess.run(["where", "nvidia-smi"], capture_output=True, text=True)
            if result.returncode == 0:
                return result.stdout.strip().split('\n')[0]
        except:
            pass
        
        logger.warning("nvidia-smi not found. GPU power management will be limited.")
        return None
    
    def _get_power_limits(self):
        try:
            result = subprocess.run([
                self.nvidia_smi_path,
                "--query-gpu=power.min_limit,power.max_limit,power.default_limit,power.limit",
                "--format=csv,noheader,nounits"
            ], capture_output=True, text=True, check=True)
            
            values = result.stdout.strip().split(', ')
            if len(values) >= 3:
                try:
                    if values[0] != '[N/A]':
                        self.min_power_limit = float(values[0])
                    if values[1] != '[N/A]':
                        self.max_power_limit = float(values[1])
                    if values[2] != '[N/A]':
                        self.default_power_limit = float(values[2])
                    if len(values) >= 4 and values[3] != '[N/A]':
                        current_limit = float(values[3])
                        logger.info(f"GPU Power Limits - Min: {self.min_power_limit}W, Max: {self.max_power_limit}W, Default: {self.default_power_limit}W, Current: {current_limit}W")
                    elif self.min_power_limit or self.max_power_limit:
                        logger.info(f"GPU Power Limits - Min: {self.min_power_limit}W, Max: {self.max_power_limit}W, Default: {self.default_power_limit}W")
                except ValueError:
                    pass
        except Exception as e:
            logger.warning(f"Could not query GPU power limits: {e}")
        
        if not self.min_power_limit:
            logger.info("GPU power limiting not supported on this device")
            self.min_power_limit = None
            self.max_power_limit = None
            self.default_power_limit = None
    
    def set_power_mode(self, profile: PowerProfile):
        if not self.nvidia_smi_path:
            return
        
        if not self.min_power_limit and not self.max_power_limit:
            logger.info("GPU power limiting not supported - skipping")
            return
        
        if profile == PowerProfile.BATTERY_SAVER:
            power_limit = self.min_power_limit or 60
        else:
            power_limit = self.max_power_limit or 105
        
        try:
            result = subprocess.run([
                self.nvidia_smi_path,
                "-pl", str(int(power_limit))
            ], capture_output=True, text=True)
            
            if "not supported" in result.stderr.lower() or "not supported" in result.stdout.lower():
                logger.info("GPU power limiting not supported on this device")
            elif result.returncode == 0:
                logger.info(f"Set GPU power limit to {power_limit}W")
            else:
                logger.warning(f"Could not set GPU power limit: {result.stderr}")
        except Exception as e:
            logger.error(f"Failed to set GPU power mode: {e}")


class ProfileManager:
    def __init__(self):
        self.config_file = Path.home() / ".battery_saver_config.json"
        self.profiles = self._load_profiles()
        self.current_profile = PowerProfile.PERFORMANCE
        self.last_errors = []
        
        self.power_manager = PowerManager()
        self.display_manager = DisplayManager()
        self.process_manager = ProcessManager()
        self.gpu_manager = GPUManager()
    
    def _load_profiles(self) -> Dict:
        default_profiles = {
            PowerProfile.BATTERY_SAVER.value: {
                "brightness": 40,
                "refresh_rate": 60,
                "kill_bloatware": True,
                "gpu_power_limit": 60
            },
            PowerProfile.PERFORMANCE.value: {
                "brightness": 80,
                "refresh_rate": 240,
                "kill_bloatware": False,
                "gpu_power_limit": 105
            }
        }
        
        if self.config_file.exists():
            try:
                with open(self.config_file, 'r') as f:
                    return json.load(f)
            except:
                pass
        
        return default_profiles
    
    def save_profiles(self):
        with open(self.config_file, 'w') as f:
            json.dump(self.profiles, f, indent=2)
    
    def apply_profile(self, profile: PowerProfile):
        settings = self.profiles[profile.value]
        self.last_errors = []
        successes = []
        
        try:
            self.power_manager.set_power_plan(profile)
            successes.append("Power plan")
        except Exception as e:
            self.last_errors.append(f"Power plan: {str(e)}")
            logger.error(f"Failed to set power plan: {e}")
        
        try:
            self.display_manager.set_brightness(settings["brightness"])
            successes.append("Brightness")
        except Exception as e:
            error_msg = str(e)
            if "0x8004100c" in error_msg or "WMI" in error_msg:
                self.last_errors.append("Brightness: Not supported on this display")
            else:
                self.last_errors.append(f"Brightness: {error_msg}")
        
        if settings.get("kill_bloatware", False):
            try:
                killed = self.process_manager.kill_bloatware()
                if killed:
                    successes.append(f"Killed {len(killed)} processes")
            except Exception as e:
                self.last_errors.append(f"Process management: {str(e)}")
        
        try:
            self.gpu_manager.set_power_mode(profile)
            if self.gpu_manager.min_power_limit or self.gpu_manager.max_power_limit:
                successes.append("GPU power")
        except Exception as e:
            if "not supported" not in str(e).lower():
                self.last_errors.append(f"GPU: {str(e)}")
        
        self.current_profile = profile
        
        if self.last_errors:
            logger.warning(f"Applied {profile.value} profile with errors: {', '.join(self.last_errors)}")
        else:
            logger.info(f"Successfully applied {profile.value} profile")
        
        return successes, self.last_errors


class BatterySaverApp:
    def __init__(self):
        self.profile_manager = ProfileManager()
        self.root = tk.Tk()
        self.setup_ui()
        self.check_admin()
    
    def check_admin(self):
        try:
            is_admin = ctypes.windll.shell32.IsUserAnAdmin()
        except:
            is_admin = False
        
        if not is_admin:
            messagebox.showwarning(
                "Admin Rights Required",
                "This app requires administrator privileges for full functionality.\n"
                "Some features may not work without admin rights."
            )
    
    def setup_ui(self):
        self.root.title("Battery Saver - Zephyrus G16")
        self.root.geometry("450x400")
        self.root.resizable(False, False)
        
        style = ttk.Style()
        style.theme_use('clam')
        
        main_frame = ttk.Frame(self.root, padding="20")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Configure grid weights for expansion
        self.root.rowconfigure(0, weight=1)
        self.root.columnconfigure(0, weight=1)
        main_frame.rowconfigure(4, weight=1)
        
        title_label = ttk.Label(
            main_frame,
            text="Power Profile Manager",
            font=('Segoe UI', 16, 'bold')
        )
        title_label.grid(row=0, column=0, columnspan=2, pady=10)
        
        self.status_label = ttk.Label(
            main_frame,
            text=f"Current Profile: {self.profile_manager.current_profile.value.upper()}",
            font=('Segoe UI', 11)
        )
        self.status_label.grid(row=1, column=0, columnspan=2, pady=10)
        
        battery_button = ttk.Button(
            main_frame,
            text="🔋 Battery Saver",
            command=lambda: self.switch_profile(PowerProfile.BATTERY_SAVER),
            width=20
        )
        battery_button.grid(row=2, column=0, padx=5, pady=10)
        
        performance_button = ttk.Button(
            main_frame,
            text="⚡ Performance",
            command=lambda: self.switch_profile(PowerProfile.PERFORMANCE),
            width=20
        )
        performance_button.grid(row=2, column=1, padx=5, pady=10)
        
        ttk.Separator(main_frame, orient='horizontal').grid(
            row=3, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=10
        )
        
        details_frame = ttk.LabelFrame(main_frame, text="Profile Settings", padding="10")
        details_frame.grid(row=4, column=0, columnspan=2, sticky=(tk.W, tk.E, tk.N, tk.S), pady=10)
        
        self.details_text = tk.Text(details_frame, height=8, width=50, state='disabled')
        self.details_text.pack(fill=tk.BOTH, expand=True)
        
        self.update_details()
        
        self.progress_bar = ttk.Progressbar(
            main_frame,
            mode='indeterminate',
            length=360
        )
        self.progress_bar.grid(row=5, column=0, columnspan=2, pady=10)
        self.progress_bar.grid_remove()
    
    def update_details(self):
        profile = self.profile_manager.current_profile
        settings = self.profile_manager.profiles[profile.value]
        
        gpu_manager = self.profile_manager.gpu_manager
        if profile == PowerProfile.BATTERY_SAVER:
            actual_gpu_limit = gpu_manager.min_power_limit or settings['gpu_power_limit']
        else:
            actual_gpu_limit = gpu_manager.max_power_limit or settings['gpu_power_limit']
        
        details = f"""Brightness: {settings['brightness']}%
Target Refresh Rate: {settings['refresh_rate']}Hz
Kill Bloatware: {'Yes' if settings['kill_bloatware'] else 'No'}
GPU Power Limit: {actual_gpu_limit}W"""
        
        if gpu_manager.min_power_limit and gpu_manager.max_power_limit:
            details += f"\n(GPU Range: {gpu_manager.min_power_limit}W - {gpu_manager.max_power_limit}W)"
        
        self.details_text.config(state='normal')
        self.details_text.delete(1.0, tk.END)
        self.details_text.insert(1.0, details)
        self.details_text.config(state='disabled')
    
    def switch_profile(self, profile: PowerProfile):
        self.progress_bar.grid()
        self.progress_bar.start()
        
        def apply():
            try:
                import pythoncom
                pythoncom.CoInitialize()
                
                successes, errors = self.profile_manager.apply_profile(profile)
                self.root.after(0, self.on_profile_applied, profile, successes, errors)
                
                pythoncom.CoUninitialize()
            except Exception as e:
                self.root.after(0, self.on_profile_error, str(e))
        
        thread = threading.Thread(target=apply)
        thread.daemon = True
        thread.start()
    
    def on_profile_applied(self, profile: PowerProfile, successes: List[str], errors: List[str]):
        self.progress_bar.stop()
        self.progress_bar.grid_remove()
        self.status_label.config(text=f"Current Profile: {profile.value.upper()}")
        self.update_details()
        
        if errors:
            title = "Partial Success"
            icon = messagebox.WARNING
            message = f"{profile.value.title()} profile partially applied.\n\n"
            if successes:
                message += f"✓ Succeeded: {', '.join(successes)}\n\n"
            message += f"✗ Failed:\n" + "\n".join(f"  • {error}" for error in errors)
            messagebox.showwarning(title, message)
        else:
            messagebox.showinfo("Success", f"{profile.value.title()} profile applied successfully!\n\n✓ " + "\n✓ ".join(successes))
    
    def on_profile_error(self, error_msg: str):
        self.progress_bar.stop()
        self.progress_bar.grid_remove()
        messagebox.showerror("Error", f"Failed to apply profile: {error_msg}")
    
    def run(self):
        self.root.mainloop()


def main():
    if sys.platform != "win32":
        print("This application only works on Windows.")
        sys.exit(1)
    
    app = BatterySaverApp()
    app.run()


if __name__ == "__main__":
    main()