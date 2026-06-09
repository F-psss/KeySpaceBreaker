#!/usr/bin/env python3
"""
Генератор тестовых данных для дешифратора.

На каждый кейс создаёт три файла в tests/data/:
  <name>-in.txt    — шифротекст (вход для клиента)
  <name>-sol.txt   — эталонная расшифровка
  <name>-meta.json — параметры запуска и ожидаемый ключ

Запуск:  python3 gen_tests.py [output_dir]
По умолчанию output_dir = tests/data
"""

import json
import os
import sys

OUTPUT_DIR = sys.argv[1] if len(sys.argv) > 1 else "tests/data"


def caesar_encrypt(text, shift):
    out = []
    for c in text:
        if c.isalpha():
            base = ord('A') if c.isupper() else ord('a')
            out.append(chr((ord(c) - base + shift) % 26 + base))
        else:
            out.append(c)
    return ''.join(out)


def vigenere_encrypt(text, key):
    out = []
    key = key.upper()
    ki = 0
    for c in text:
        if c.isalpha():
            base = ord('A') if c.isupper() else ord('a')
            shift = ord(key[ki % len(key)]) - ord('A')
            out.append(chr((ord(c) - base + shift) % 26 + base))
            ki += 1
        else:
            out.append(c)
    return ''.join(out)


def shift_to_key_letter(shift):
    # Клиент возвращает ключ Caesar как одну букву (A=0, B=1, ...)
    return chr(ord('A') + shift % 26)


def write_case(name, ciphertext, plaintext, meta):
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    with open(os.path.join(OUTPUT_DIR, f"{name}-in.txt"), "w") as f:
        f.write(ciphertext)
    with open(os.path.join(OUTPUT_DIR, f"{name}-sol.txt"), "w") as f:
        f.write(plaintext)
    with open(os.path.join(OUTPUT_DIR, f"{name}-meta.json"), "w") as f:
        json.dump(meta, f, indent=2)
    print(f"  {name}")


# Эталонные тексты разной длины. Используем lowercase т.к. клиент так выводит.
SHORT = "the quick brown fox jumps over the lazy dog"

MEDIUM = (
    "after assuming control of government caesar carried out various reforms "
    "and public works the senate feared his growing power and on the ides of "
    "march a group of senators stabbed him to death in the theatre of pompey"
)

LONG = (
    """It was a key. An ordinary key. I found it when I was getting your books ready for Goodwill. It was taped to the inside of the back cover of one of your music notebooks and looked like one of those thin, metal keys found on luggage. We never locked our luggage when we traveled. I wanted to, but you insisted it would never deter anyone. You used to laugh and say it would do just the opposite. That if you were a luggage handler at the airport, you would “accidentally” break all the locks you came across, just to see what was so precious inside. You also worried we would lose the keys. I wanted to remind you that I never lost anything, but I didn’t argue with you. I never liked arguing with you.
What could this key possibly unlock? We had joint accounts, joint trusts, joint everything. I had been managing our lives entirely. The practical, boring stuff. Oh, I didn’t mind. You brought so much beauty to my life, to so many other people’s lives. I thought it was funny to be your stay-at-home husband. And I was proud. So proud. How many times I sat in the front row watching you conduct your own music, swiveling my head all around to make sure everyone saw me and that everyone knew you were mine. Your hands would dance in the air, and your wild mane would quiver every time you jumped to the music. Such passion.
You stopped using batons because like everything else, you kept losing them. I thought your ability to lose things was endearing. You didn’t need a baton. You were mesmerizing to watch without any implements. I could watch you like that forever and never tire. Your Requiem, that was the last piece you conducted, right before the bad fall. If you remembered, you would think it was fitting for that to be your last. Just like Mozart, your favorite composer. And just as good too. Could serve as the end of the Requiem Mozart never finished.
Towards the end, oh, the end, you used to lie in bed eyes closed, and when you couldn’t move your hands anymore, your eyes would move under the lids, and I knew. I knew you were conducting. How I wished I could hear what you were hearing. I could never hum a tune, never see a movie in my mind. You always thought it was so fascinating, my aphantasia. I didn’t think there was anything interesting about it at all. What a pair we made: a man who lived in his mind and a man unable to picture anything there.
We didn’t have any secrets from each other. At least I didn’t have any from you. Why would you want to lock something away and not tell me about it? I didn’t want to believe anything sinister; I trusted you completely. Even though you had all those groupies and I knew that tuba player was in love with you. How his face would melt when you climbed up on the podium and raised your hands.
You were the looker of the two of us: tall, broad-shouldered, olive-skinned. You had the most intense eyes I had ever seen. I couldn’t believe you were interested in me. When I asked you what you saw in average-ole me, you said kindness and pure honesty. You said you were starved for these, and I felt so sorry for you. Sorry that you hadn’t experienced such basics. In my vows to you, I promised never to lie to you and never to be unkind. And you said you were mine and mine alone for the duration. And I believed you. I never worried. What could you possibly need to hide from me with a key?
I searched the house and didn’t find anything locked. I searched both of your workrooms, first the one with the heavy oak table where you would start each of your compositions before transferring it to the computer in the recording room. I found other notebooks, just like this one. Yet none of them had a hidden key. And there was nothing locked anywhere I looked. I began to think there was nothing for me to find. But after you were gone, there was nothing else for me to do. I didn’t know what I hoped to find. I had already cleaned the house multiple times top to bottom and organized and reorganized everything that could be given away. I needed to do something else. I began to think I would never find a lock for that key although I was so grateful for your leaving me this mystery. It kept me occupied for weeks, and that was good.
I found it when I had stopped looking. I was flying to Reykjavik to visit my sister for the holidays, and I went to the basement to find my favorite neon-green suitcase. Rummaging through our metal rack of traveling bags, so many, too many, I came across one I had never seen before. It was a small black chalice case. Didn’t look like a suitcase at all. Looked more like a small safe. It was locked. My heart beat wildly as I ran upstairs for the key. I must have stood there in front of the locked case for a good five minutes, thinking all kinds of things I never thought before. I considered not opening it just to respect your privacy and from fear of what secret I might uncover. After all, this was something you never told me about. It was clearly something you didn’t want me to find.
And then curiosity got the better of me. I made a promise to love you no matter what I found. The key slid into the lock, and I turned it once, unlocking the case. Trembling with emotion, I lifted the lid. There, on a padded bed, fastened by a tight band so it would remain secure, was a gold baton, the first present I gave you those many, many years ago. And I wept."""
)


def main():
    print(f"Generating into {OUTPUT_DIR}/")
    idx = 1

    # --- Caesar: разные сдвиги ---
    for shift in [1, 3, 7, 13, 19, 25]:
        name = f"t-{idx:02d}-caesar-shift{shift}"
        ct = caesar_encrypt(MEDIUM, shift)
        write_case(name, ct, MEDIUM, {
            "cipher": "caesar",
            "mode": "brute",
            "key_length": 1,
            "noise": 0.0,
            "expected_key": shift_to_key_letter(shift),
        })
        idx += 1

    # --- Caesar: короткий текст ---
    name = f"t-{idx:02d}-caesar-short"
    ct = caesar_encrypt(SHORT, 5)
    write_case(name, ct, SHORT, {
        "cipher": "caesar",
        "mode": "brute",
        "key_length": 1,
        "noise": 0.0,
        "expected_key": shift_to_key_letter(5),
    })
    idx += 1

    # --- Vigenere brute: ключи длины 2,3,4 ---
    for key in ["AB", "KEY", "CODE"]:
        name = f"t-{idx:02d}-vigenere-brute-{key.lower()}"
        ct = vigenere_encrypt(MEDIUM, key)
        write_case(name, ct, MEDIUM, {
            "cipher": "vigenere",
            "mode": "brute",
            "key_length": len(key),
            "noise": 0.0,
            "expected_key": key,
        })
        idx += 1

    # --- Vigenere fast: длинный текст, ключи 3-6 ---
    for key in ["KEY", "CODE", "MONEY"]:
        name = f"t-{idx:02d}-vigenere-fast-{key.lower()}"
        ct = vigenere_encrypt(LONG, key)
        write_case(name, ct, LONG, {
            "cipher": "vigenere",
            "mode": "fast",
            "key_length": len(key),
            "noise": 0.1,
            "expected_key": key,
        })
        idx += 1

    print(f"Done. {idx - 1} cases generated.")


if __name__ == "__main__":
    main()
