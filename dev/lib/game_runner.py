#!/usr/bin/env python3
"""
1984 Hack game automation for testing
"""

import os
import sys
import time
import random
from typing import Tuple, Optional
import pexpect

# Movement keys: 8 directions, walk and run
MOVEMENT_KEYS = 'hjklyubnHJKLYUBN'
# After each move, read output until the game is quiet this long (seconds).
# Draining replaces fixed sleeps and keeps the pty from filling up, which
# would block the game and then our own writes.
KEY_SETTLE = 0.03
# Enhanced mode: per-move chance of a stairs key and of a terminal resize
STAIRS_CHANCE = 0.02
RESIZE_CHANCE = 0.002


class HackGameRunner:
    """Reliable 1984 Hack game automation with enhanced inventory and save/resume testing"""
    
    def __init__(self, binary_path: str, debug: bool = False, enhanced_mode: bool = False,
                 seed: Optional[int] = None):
        self.binary_path = binary_path
        self.debug = debug
        self.enhanced_mode = enhanced_mode
        self.seed = seed
        # Private RNG so a given seed always yields the same key sequence
        self.rng = random.Random(seed)
        self.child: Optional[pexpect.spawn] = None
        self.current_inventory = []  # Track inventory state
        self.fail_reason = ""        # Why the last session reported failure
        self.save_file_path = None   # Track save file location
        self.resizes = 0             # Resize cycles attempted this session
        self.resizes_seen = 0        # ...that reached "TERMINAL TOO SMALL"
        self.depth = 1               # Dungeon level, tracked from stairs use
        
    def log(self, msg: str) -> None:
        if self.debug:
            print(f"[GAME] {msg}", file=sys.stderr)
            
    def run_session(self, steps: int = 200, player_name: str = "TestPlayer") -> Tuple[bool, int]:
        """Run game session: ./hack -> n -> space -> space -> moves -> i -> space -> moves -> d -> a -> moves -> Q -> y"""
        try:
            # Start game (only if child doesn't already exist)
            if not hasattr(self, 'child') or self.child is None:
                cmd = f'{self.binary_path} -D'
                self.log(f"Starting: {cmd}")
                self.child = pexpect.spawn(cmd, timeout=10)
                if self.debug:
                    self.child.logfile_read = sys.stderr.buffer
            else:
                self.log(f"Using existing child process")
                
            # Handle startup sequence: experienced player question
            try:
                self.child.expect('Are you an experienced player', timeout=5)
                self.child.send('n')  # Choose "no" - let game pick character
                
                # Wait for character selection message and any key prompt
                self.child.expect(['choose a character', 'This game you will be', 'choose a character for you.'], timeout=5)
                self.child.send(' ')  # Press space to continue after character selection
            except pexpect.TIMEOUT:
                self.fail_reason = "startup timeout"
                self.log("Timeout during startup sequence")
                return False, -1
            except pexpect.EOF:
                self.fail_reason = "EOF during startup"
                self.log("Unexpected EOF during startup")
                return False, -1
                
            # Wait for game start and clear intro
            self.child.expect(['--More--', 'Hit space', '@', 'Dlvl:', pexpect.TIMEOUT], timeout=8)
            self.child.send(' ')  # First space
            time.sleep(0.5)
            self.child.send(' ')  # Second space (clear any remaining text)
            time.sleep(0.5)
            
            # Phase 1: Move around (with optional save/resume testing)
            # Test save/resume mid-game if enhanced mode enabled
            if self.enhanced_mode and self.rng.random() < 0.3:  # 30% chance
                # Move some steps before save
                for _ in range(min(50, steps)):
                    self._move()
                    
                # Test save functionality
                if self._test_save_game():
                    # If save successful, test resume (end this session)
                    return True, 0
            
            # Standard movement phase
            # Honor --steps; enhanced mode varies it between 0.5x and 1.5x
            if self.enhanced_mode:
                move_count = self.rng.randint(max(1, steps // 2), max(1, steps * 3 // 2))
            else:
                move_count = steps
            self.last_move_count = move_count
            self.log(f"Movement phase: {move_count} moves (steps={steps}, seed={self.seed})")
            for _ in range(move_count):
                self._move()
                
            # Phase 2: Enhanced inventory testing (if enabled)
            if self.enhanced_mode and self.rng.random() < 0.6:  # 60% chance
                self._test_enhanced_inventory()
            else:
                # Basic inventory test
                self.child.send('i')
                time.sleep(0.5)
                self.child.send(' ')  # Space to close inventory
                time.sleep(0.5)
            
            # Phase 3: Move some more (3 moves)
            for _ in range(3):
                self._move()
                
            # Phase 4: Enhanced item operations (drop, pickup, apply)
            if self.enhanced_mode and self.rng.random() < 0.5:  # 50% chance
                self._test_smart_item_operations()
            else:
                # Basic drop and apply
                self.child.send('d')  # Drop
                time.sleep(0.2)
                self.child.send('a')  # Apply  
                time.sleep(0.2)
            
            # Phase 5: Final moves (3 more)
            for _ in range(3):
                self._move()
                
            # Quit sequence
            # Clear any pending prompt (--More--, "What do you want to drop?",
            # death screens in wizard mode) that would swallow the 'Q'.
            # Return ends text-entry prompts (getlin echoes ESC) and the
            # "Hit return to continue" after escaping the dungeon.
            try:
                for attempt in range(8):
                    self.child.send(' \x1b\r')
                    time.sleep(0.3)
                    self.child.send('Q')
                    try:
                        self.child.expect(['Really quit', 'quit'], timeout=2)
                        break
                    except pexpect.TIMEOUT:
                        self.log(f"No quit prompt (attempt {attempt + 1}); retrying")
                else:
                    raise pexpect.TIMEOUT("no quit prompt after retries")
                self.child.send('y')
                time.sleep(0.5)
                # The exit may be preceded by --More-- pages (tombstone/score)
                for _ in range(6):
                    if self.child.expect([pexpect.EOF, '--More--'], timeout=5) == 0:
                        break
                    self.child.send(' ')
                else:
                    raise pexpect.TIMEOUT("game did not exit after quit")
                # If we get here, clean exit
                return True, self._exit_status()
            except pexpect.EOF:
                # Game already ended (e.g. the player died during the moves):
                # not a harness failure; sanitizer/signal checks still apply.
                self.log("Game exited before/during quit sequence")
                return True, self._exit_status()
            except pexpect.TIMEOUT:
                tail = (self.child.before or b"")[-200:] if isinstance(self.child.before, bytes) else str(self.child.before)[-200:]
                self.fail_reason = f"quit sequence timed out (last output: {tail!r})"
                self.log("Quit sequence timed out; killing game")
                self.child.terminate(force=True)
                return False, -2
            
        except pexpect.EOF:
            # Game ended during the moves; exit status and signals decide
            self.log("Game exited during play")
            return True, self._exit_status()
        except Exception as e:
            self.fail_reason = f"exception: {e}"
            self.log(f"Session failed: {e}")
            if self.child and self.child.isalive():
                self.child.terminate(force=True)
            return False, -1

    def _exit_status(self) -> int:
        """Reap the game and return its exit status (signals: see signalstatus)."""
        self.child.close()
        return self.child.exitstatus if self.child.exitstatus is not None else 0

    def _drain(self, settle: float = KEY_SETTLE) -> None:
        """Read output until quiet, dismissing --More-- (and "Hit return",
        shown when the game ends) so later keys reach the command loop
        instead of being swallowed by the prompt."""
        for _ in range(20):
            i = self.child.expect(['--More--', 'Hit return to continue', pexpect.TIMEOUT],
                                  timeout=settle)
            if i == 2:
                return
            self.child.send(' ' if i == 0 else '\r')

    def _move(self) -> None:
        """One step: usually a movement key; in enhanced mode sometimes a
        stairs key or a terminal resize cycle.  All choices use self.rng."""
        if self.enhanced_mode:
            roll = self.rng.random()
            if roll < RESIZE_CHANCE:
                self._resize_cycle()
                return
            if roll < RESIZE_CHANCE + STAIRS_CHANCE:
                self._stairs()
                return
        self.child.send(self.rng.choice(MOVEMENT_KEYS))
        self._drain()

    def _stairs(self) -> None:
        """Send '>' or '<'.  '<' only below level 1: the game starts on the
        up stairs, and '<' there leaves the dungeon and ends the run.

        A level change clears the screen; a stairs key off the stairs says
        "You can't go down/up here".  Watching for either tracks the depth
        without parsing the partially repainted status line.
        """
        key = self.rng.choice('<>') if self.depth > 1 else '>'
        self.child.send(key)
        i = self.child.expect([r'\x1b\[2J', "can't go (down|up) here", pexpect.TIMEOUT],
                              timeout=0.5)
        if i == 0:
            self.depth += 1 if key == '>' else -1
            self.log(f"Stairs '{key}': now on level {self.depth}")
        self._drain()

    def _resize_cycle(self) -> None:
        """Shrink the pty below 80x24 and restore it (the #21 path).

        check_resize() runs once per main-loop turn, so after shrinking we
        send one command ('s', search) to get there.  The game then pauses
        on "TERMINAL TOO SMALL ... press any key"; restore the size first,
        then press a key, or the game keeps waiting.
        """
        if self.rng.random() < 0.5:
            rows, cols = self.rng.randint(5, 23), self.rng.randint(20, 80)
        else:
            rows, cols = self.rng.randint(5, 24), self.rng.randint(20, 79)
        self.resizes += 1
        self.child.setwinsize(rows, cols)
        self.child.send('s')
        seen = False
        for _ in range(10):
            i = self.child.expect(['TERMINAL TOO SMALL', '--More--', pexpect.TIMEOUT], timeout=3)
            if i == 0:
                seen = True
                break
            if i == 2:
                break
            self.child.send(' ')
        self.child.setwinsize(24, 80)
        if seen:
            self.resizes_seen += 1
            self.child.expect('press any key', timeout=3)
            self.child.send(' ')
        self.log(f"Resize to {cols}x{rows}: {'TERMINAL TOO SMALL shown' if seen else 'no prompt'}")
        self._drain()

    def __del__(self):
        if self.child and self.child.isalive():
            try:
                self.child.terminate(force=True)
            except:
                pass
                
    def _test_enhanced_inventory(self) -> None:
        """Enhanced inventory testing with item parsing and interaction"""
        self.log("Running enhanced inventory tests")
        
        try:
            # Full inventory display
            self.child.send('i')
            time.sleep(0.5)
            
            # Try to capture inventory content
            try:
                self.child.expect(['--More--', pexpect.TIMEOUT], timeout=1)
                self.child.send(' ')  # Clear more prompt
                time.sleep(0.3)
            except pexpect.TIMEOUT:
                pass  # No more prompt, continue
                
            # Test selective inventory commands
            inventory_tests = ['I$', 'I*', 'IU']  # Money, gems, unpaid
            chosen_test = self.rng.choice(inventory_tests)
            
            self.child.send(chosen_test)
            time.sleep(0.4)
            
            # Clear any output
            try:
                self.child.expect(['--More--', pexpect.TIMEOUT], timeout=1)
                self.child.send(' ')
                time.sleep(0.3)
            except pexpect.TIMEOUT:
                pass
                
            # Test item interaction if we have items
            if self.rng.random() < 0.4:  # 40% chance to try item interaction
                self._test_item_interaction()
                
        except Exception as e:
            self.log(f"Enhanced inventory test failed: {e}")
            # Fallback to space to clear any prompts
            self.child.send(' ')
            time.sleep(0.2)
            
    def _test_item_interaction(self) -> None:
        """Test wielding, wearing, and using items"""
        self.log("Testing item interactions")
        
        # List of common starting item letters in 1984 Hack
        possible_items = ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h']
        test_item = self.rng.choice(possible_items)
        
        # Test different item operations
        operations = [
            ('w', 'wield'),     # Wield weapon
            ('W', 'wear'),      # Wear armor  
            ('P', 'put on'),    # Put on ring
            ('r', 'read'),      # Read scroll
            ('q', 'quaff'),     # Drink potion
        ]
        
        operation, desc = self.rng.choice(operations)
        
        try:
            self.child.send(operation)
            time.sleep(0.3)
            
            # Try to interact with the chosen item
            self.child.send(test_item)
            time.sleep(0.4)
            
            # Handle common responses
            try:
                self.child.expect([
                    'What do you want to', 
                    'You don\'t have', 
                    'You can\'t',
                    '--More--',
                    pexpect.TIMEOUT
                ], timeout=1)
                
                # Send space or escape to clear any prompts
                self.child.send(' ')
                time.sleep(0.2)
                
            except pexpect.TIMEOUT:
                pass  # Operation completed or failed silently
                
        except Exception as e:
            self.log(f"Item interaction test ({operation}) failed: {e}")
            # Clear any hanging prompts
            self.child.send('\x1b')  # Escape key
            time.sleep(0.2)
            
    def _test_smart_item_operations(self) -> None:
        """Test smart dropping, pickup, and item manipulation"""
        self.log("Testing smart item operations")
        
        operations = [
            self._test_smart_drop,
            self._test_pickup_operations,
            self._test_examine_ground,
            self._test_multi_drop
        ]
        
        # Randomly choose 1-2 operations to test
        num_ops = self.rng.randint(1, 2)
        chosen_ops = self.rng.sample(operations, num_ops)
        
        for operation in chosen_ops:
            try:
                operation()
            except Exception as e:
                self.log(f"Smart operation failed: {e}")
                # Clear any prompts
                self.child.send(' ')
                time.sleep(0.2)
                
    def _test_smart_drop(self) -> None:
        """Test targeted item dropping with specific item letters"""
        self.log("Testing smart drop operations")
        
        # Test dropping specific items by letter
        test_items = ['a', 'b', 'c', 'd', 'e']  # Common starting items
        item_to_drop = self.rng.choice(test_items)
        
        try:
            self.child.send('d')  # Drop command
            time.sleep(0.3)
            
            # Try to drop the specific item
            self.child.send(item_to_drop)
            time.sleep(0.4)
            
            # Handle possible responses
            try:
                self.child.expect([
                    'You don\'t have',
                    'What do you want to drop',
                    'Dropped',
                    '--More--',
                    pexpect.TIMEOUT
                ], timeout=1)
                
                self.child.send(' ')  # Clear any prompt
                time.sleep(0.2)
                
            except pexpect.TIMEOUT:
                pass
                
        except Exception as e:
            self.log(f"Smart drop failed: {e}")
            self.child.send(' ')
            time.sleep(0.2)
            
    def _test_pickup_operations(self) -> None:
        """Test pickup and ground examination"""
        self.log("Testing pickup operations")
        
        try:
            # Test pickup command
            self.child.send(',')
            time.sleep(0.4)
            
            # Handle pickup responses
            try:
                self.child.expect([
                    'There is nothing here',
                    'Pick up',
                    'You cannot carry',
                    '--More--',
                    pexpect.TIMEOUT
                ], timeout=1)
                
                # Send space or 'n' to handle prompts
                response = self.rng.choice([' ', 'n', 'y'])
                self.child.send(response)
                time.sleep(0.3)
                
            except pexpect.TIMEOUT:
                pass
                
        except Exception as e:
            self.log(f"Pickup test failed: {e}")
            self.child.send(' ')
            time.sleep(0.2)
            
    def _test_examine_ground(self) -> None:
        """Test examining what's on the ground"""
        self.log("Testing ground examination")
        
        try:
            # Look at what's here
            self.child.send(':')
            time.sleep(0.4)
            
            # Clear any output
            self.child.send(' ')
            time.sleep(0.2)
            
        except Exception as e:
            self.log(f"Ground examination failed: {e}")
            self.child.send(' ')
            time.sleep(0.2)
            
    def _test_multi_drop(self) -> None:
        """Test multi-item drop command (D)"""
        self.log("Testing multi-drop operations")
        
        try:
            # Multi-drop command
            self.child.send('D')
            time.sleep(0.3)
            
            # Try different drop patterns
            drop_patterns = ['!', '%', '*', 'au', '']  # Potions, food, all, all unpaid, none
            pattern = self.rng.choice(drop_patterns)
            
            self.child.send(pattern)
            time.sleep(0.4)
            
            # Handle confirmation prompts
            try:
                self.child.expect([
                    'Drop',
                    'Nothing to drop',
                    'What kinds',
                    '--More--',
                    pexpect.TIMEOUT
                ], timeout=1)
                
                # Send space or enter to confirm/cancel
                self.child.send(' ')
                time.sleep(0.3)
                
            except pexpect.TIMEOUT:
                pass
                
        except Exception as e:
            self.log(f"Multi-drop test failed: {e}")
            self.child.send(' ')
            time.sleep(0.2)
            
    def _test_save_game(self) -> bool:
        """Test save game functionality (S command)"""
        self.log("Testing save game functionality")
        
        try:
            # Send save command
            self.child.send('S')
            time.sleep(0.5)
            
            # Handle save confirmation/prompts
            try:
                self.child.expect([
                    'Save game',
                    'Saving',
                    'saved',
                    'Cannot save',
                    '--More--',
                    pexpect.TIMEOUT
                ], timeout=3)
                
                # Confirm save if prompted
                self.child.send('y')  # Yes to save
                time.sleep(0.5)
                
                # Check if save completed
                try:
                    self.child.expect([
                        'saved',
                        'Saved',
                        'Game saved',
                        pexpect.EOF,  # Game might exit after save
                        pexpect.TIMEOUT
                    ], timeout=2)
                    
                    self.log("Save game successful")
                    return True
                    
                except pexpect.TIMEOUT:
                    self.log("Save game timeout - unclear result")
                    return False
                    
            except pexpect.TIMEOUT:
                self.log("Save game command timeout")
                return False
                
        except Exception as e:
            self.log(f"Save game failed: {e}")
            # Try to clear any prompts
            self.child.send(' ')
            time.sleep(0.2)
            return False