/*
  ระบบวัด Water Activity (aw) พร้อมเมนูเลือกโหมดผ่านจอ LCD 16x2 (I2C) + ไอคอนบนจอ TFT
  ควบคุมด้วยปุ่มแยกภายนอก 2 ปุ่ม (GPIO32=UP, GPIO33=DOWN)
  + ปล่อย Wi-Fi ภายในตัว (Access Point) เพื่อดูกราฟ/ค่าผ่าน Web Dashboard

  *** ไฟล์นี้ = เวอร์ชัน "ค่าดิบ" (RAW) — AW_RAW_MODE = 1 ***
  ค่า aw ที่แสดงผล/บันทึก = %RH ดิบจากเซนเซอร์ / 100 ตรง ๆ ไม่ผ่านสมการคาลิเบรต ใช้เทียบเคียง/อ้างอิงกับเวอร์ชันคาลิเบรต
  (ซ่อนแผงคาลิเบรตบนเว็บ, ปิดการแก้จุดคาลิเบรต, ไม่โหลด/ไม่แตะจุดคาลิเบรตที่เก็บใน NVS)
  ไฟล์คู่กัน: www.ino = เวอร์ชันคาลิเบรต — สองไฟล์ต่างกันแค่บรรทัด #define AW_RAW_MODE

  *** เว็บแดชบอร์ด = สำหรับดูค่า/กราฟ/ export เท่านั้น ไม่มีปุ่มควบคุมเครื่องจากเว็บอีกต่อไป ***
  ตัดออกแล้ว: แผง "ควบคุมเครื่องจากเว็บ" (สั่งวัด/ทำนาย/เปรียบเทียบ/ยกเลิกจากระยะไกล — เอนด์พอยต์ /cmd/measure,
  /cmd/predict, /cmd/compare, /cmd/cancel) และปุ่ม/ช่องติ๊กควบคุมฮีตเตอร์จากเว็บ (/heater, /autoheater)
  — ฮีตเตอร์อัตโนมัติยังทำงานเหมือนเดิม เว็บแค่โชว์แบนเนอร์เตือนตอนฮีตเตอร์เปิดอยู่ ควบคุมเครื่องได้จากปุ่มที่ตัวเครื่องเท่านั้น

  *** v12: สิ่งที่เพิ่ม/แก้ในเวอร์ชันนี้ ***
  1) ฮีตเตอร์ในตัวเซนเซอร์ SHT ก่อน/หลังวัด (sensor conditioning) — ทุกโหมดวัด (Start / Predict / Compare)
       ก่อนวัด : ฮีต SHT_PREHEAT_MS แล้วรอให้เซนเซอร์เย็นลง SHT_PRECOOL_MS ค่อยเริ่มจับเวลา/เก็บกราฟจริง
       หลังวัด : ฮีต SHT_POSTHEAT_MS (ไล่ไอน้ำตกค้างจากตัวอย่างเดิม) ก่อนกลับเมนู  (กดค้าง 1 ปุ่ม = ข้ามได้)
       ช่วงฮีต/รอเย็น เว็บจะไม่บันทึกค่าลงกราฟ/CSV (ดูฟิลด์ phase ใน /data)
  2) ป็อปอัปบนจอบอร์ดเมื่อ "กราฟนิ่ง" (ช่วงกว้างของเส้น aw บนกราฟ <= SAVE_PROMPT_TOL ~ 0.0005-0.001)
       - โหมดวัดปกติ: ถาม "Save this result?" แต่การวัด/กราฟเดินต่อไปเรื่อย ๆ จนกว่าจะกด Save หรือ Don't save
       - โหมดเปรียบเทียบ: ตัวอย่าง A ถาม "Measure B next?" (Yes = ไปวัด B ต่อ / No = ยกเลิก)
                          ตัวอย่าง B ถาม "Save A and B?" แล้วโชว์หน้าสรุปผลเทียบ A vs B
  3) แก้บั๊กเดิม: STAB_BUF_CAP (200) เล็กกว่าจำนวนตัวอย่างที่ต้องใช้ในหน้าต่างนิ่ง (300) ทำให้เดิมค่าไม่เคย "นิ่ง" ได้เลย
  4) ใบรายงาน (COA) บนเว็บไม่แนบกราฟแล้ว / กราฟส่งออกเป็น PNG พื้นขาวแบบเอกสารทางการ อ้างอิงเลขที่กราฟได้

  *** v13: ความเสถียรของสัญญาณ / Wi-Fi / ไฟเลี้ยงตอนบูต + ทำนายค่าสมดุล (Predict) ***
  1) I2C / LCD: จอ LCD ผ่านคลาส SafeLCD (เขียนเฉพาะตัวอักษรที่เปลี่ยน, รีเฟรชทั้งจอทุก 2 วิ, รีเซ็ตจอทุก 60 วิ, ตรวจ ACK)
       ล้างบัส I2C อัตโนมัติเมื่อ SDA ค้าง / อ่าน SHT ล้มเหลวติดกัน, ลดความเร็วบัสเหลือ 50 kHz + timeout, อ่าน SHT ซ้ำ 1 ครั้งเมื่อ CRC ผิด
       DS18B20: ตัดค่า 85.0 (ค่ารีเซ็ตตอนไฟตก) และค่ากระโดดผิดปกติ, ประกาศ fault ต่อเมื่อพลาดติดกัน 3 ครั้ง
       /data ใช้ค่า SHT ที่แคชไว้ (ไม่อ่านซ้ำซ้อนกับรอบวัด) และสร้าง JSON ด้วย snprintf (ไม่แตกเศษ heap ระยะยาว)
  2) Wi-Fi AP: ตั้งค่าครบ (โหมด/ช่อง/IP/จำนวนผู้ใช้), เลือกช่อง 1/6/11 ที่คลื่นน้อยสุดตอนบูต, ลดกำลังส่ง 15 dBm (กระแสพีคต่ำลง),
       ตรวจ AP ทุก 10 วิ ถ้าหลุดจะเปิดใหม่เอง
  3) ไฟเลี้ยงตอนบูต: ตัดเทลเทียร์เป็นอย่างแรกสุด, หน่วงเริ่มเทลเทียร์หลัง Wi-Fi ขึ้น + ค่อย ๆ เร่งกำลัง (ramp), CPU 160 MHz,
       บันทึกสาเหตุรีเซ็ต (brownout/watchdog/panic) ลง NVS แสดงใน System Health และเว็บ — ถ้ารีเซ็ตผิดปกติติดกัน 2 ครั้ง
       เข้าโหมด SAFE START (CPU 80 MHz, Wi-Fi 8.5 dBm, เริ่มเทลเทียร์ช้าลง)
  *** v14: คาลิเบรต + ชดเชยอุณหภูมิ (ห้องแอร์) ***
  - โหมดคาลิเบรต (AW_RAW_MODE 0) เปิดใช้งาน + ผู้ช่วยคาลิเบรตบนเว็บ (สร้างจุดคาลิเบรตจากรอบที่บันทึกพร้อมค่าอ้างอิง ตรวจความขัดแย้งของข้อมูลให้)
  - วัดอุณหภูมิตัวชิป SHT (ตัวเดียวกับที่ใช้อ้างอิง %RH) เทียบกับ DS18B20 (อุณหภูมิตัวอย่าง/ห้องวัด) -> ชดเชย %RH ด้วยสูตร Magnus
    (RH ที่อ่านได้อ้างอิงอุณหภูมิของชิป ไม่ใช่ของตัวอย่าง: ชิปอุ่นกว่าตัวอย่าง 1 °C = อ่านต่ำลงราว 6%) — ลำดับ: ตารางคาลิเบรต -> ชดเชยอุณหภูมิ
  *** v-cal-priority: ลำดับความสำคัญการคาลิเบต 3 ชั้น (ดูคำอธิบายเต็มที่ applyCal()) — ไม่เปลี่ยนพฤติกรรมคำนวณ แค่ทำให้โค้ด/คอมเมนต์ชัดเจนขึ้นว่าแต่ละชั้นมีที่มา/น้ำหนักความน่าเชื่อถืออย่างไร ***
  1) ใส่ค่าจากเครื่องสอบเทียบ (Quick Cal, handleCalQuick()) — แก้ตาราง calPoints โดยตรง/ถาวร น่าเชื่อถือที่สุดเพราะเทียบกับเครื่องอ้างอิงจริง
  2) เกณฑ์ที่ชดเชยอุณหภูมิแล้ว (gradientFactor(), สูตร Magnus) — คูณทับผลของชั้น 3 เสมอ ตามส่วนต่างอุณหภูมิชิป-ตัวอย่าง ณ ขณะนั้น
  3) คาลิเบตปกติ (calTableLookup(), ตาราง piecewise-linear) — ฐานล่างสุดที่รวมผลของ Quick Cal (ชั้น 1) ไว้แล้ว
  - เตือนบนจอ/เว็บเมื่อห้องเย็นกว่าเป้าหมายจนเทลเทียร์ (ทำความเย็นอย่างเดียว) ควบคุมอุณหภูมิไม่ได้
  4) Predict: กราฟบนจอแสดงเส้นประ "จุดสมดุลที่ทำนาย" + เส้นโค้งประที่คาดว่าจะไปถึง + จุดเวลา ETA, สเกลแกน Y ซูมอัตโนมัติ,
       สมการทำนายใหม่ (เฉลี่ยเป็นบล็อก + ฟิตกำลังสองน้อยสุด AR(1)/AR(2)) แม่นกว่าสูตร 3 จุดเดิมมาก และมีเครื่องหมาย ~ เมื่อยังไม่มั่นใจ
  *** v16 (ทำทีละขั้น): ขั้นที่ 1 = ตัวตรวจ "ความนิ่ง" ใหม่ (เร็วขึ้น) + ไฟ LED ความหมายเดียวกันทุกโหมด ***
  - ตัดสินความนิ่งจากค่า aw เฉลี่ยบล็อก 5 วิ: ช่วงกว้าง <= SAVE_PROMPT_TOL (0.0008 ปรับได้ 0.0005-0.001) ใน 60 วิ + ความชัน 2 นาที <= 0.0002 aw/นาที
    (เดิมต้องรอ 5 นาที + วัดขั้นต่ำ 5 นาที) วัดขั้นต่ำเหลือ 2 นาที
  - ไฟ LED: แดง = ยังเคลื่อนที่ / เหลือง = คงที่ต่อเนื่อง 20 วิ / เขียว = นิ่งครบ 60 วิ ล็อกค่าได้ — ใช้ตัวตัดสินตัวเดียวกันทั้งวัดปกติ, เปรียบเทียบ,
    คาลิเบตจากเว็บ (เดิมโหมดคาลิเบตใช้เกณฑ์หลวมกว่า 10 เท่า) และโหมด Predict ก็มีไฟแล้ว; /data มีฟิลด์ stabPhase ให้เว็บโชว์สีเดียวกัน
  *** v18: คำว่า "คงที่/นิ่ง" อิง PWM เทลเทียร์ด้วย (ทั้งโหมดวัดปกติ, เปรียบเทียบ และคาลิเบตอัตโนมัติ) ***
  - PID ทำให้อุณหภูมิแกว่งขึ้น-ลงเล็กน้อยรอบเป้าหมาย -> aw ขึ้น-ลงหน่อย ๆ ตาม จนเกณฑ์ช่วงกว้าง aw/อุณหภูมิเดิมแทบไม่เคยนับว่านิ่ง
  - ถ้า PWM เฉลี่ยรายบล็อก 5 วิ นิ่งตลอด 60 วิ (แกว่ง <= 8% และไม่ใช่ปิด/เต็มกำลัง) = ลูปควบคุมเข้าสมดุล -> ผ่อนเกณฑ์ช่วงกว้าง aw x2.5,
    ความชัน x1.5, ช่วงกว้างอุณหภูมิ x2 (ปรับที่ STAB_PWM_* ) ถ้า PWM ยังไม่นิ่งใช้เกณฑ์เข้มเดิมทุกประการ; /data มีฟิลด์ pwmSteady
  *** v16 ขั้นที่ 2: คาลิเบตอัตโนมัติ 10 รอบ (วัด/ทดสอบสลับ) ที่ 25/25/25/20/19 °C จบรอบเมื่อนิ่ง (เพดาน 30 นาที/รอบ) ***
  - รอบวัดตั้ง/เฉลี่ยจุดคาลิเบรตชั่วคราว รอบทดสอบเช็คว่าจุดนั้นให้ aw ตรงเป้า (+-0.0015) ไม่ตรง = แก้จุดใหม่ ก่อนไปรอบถัดไป
  - เก็บจุด RAW ต่ออุณหภูมิ (25/20/19 °C) ไว้ใช้ชดเชยตามอุณหภูมิในขั้นถัดไป; /admin/calmode/status มี points/type/pass/fixed/errAw
  *** v19: ค่าคงที่คำนึงถึง ripple ของ aw ที่เกิดจากอุณหภูมิแกว่งตาม PWM ***
  - เกณฑ์ช่วงกว้าง aw ตอน PWM นิ่ง = max(tol x STAB_PWM_AW_TOL_MULT, tol + STAB_PWM_AW_RIPPLE) แทนตัวคูณอย่างเดียว (ไม่พังเมื่อปรับ SAVE_PROMPT_TOL ให้เข้ม)
  - รอบทดสอบคาลิเบตอัตโนมัติ: ถ้า PWM นิ่งยอมคลาดเพิ่ม AUTOCAL_VERIFY_PWM_EXTRA_AW; ป็อปอัปบนเว็บใช้สูตรเดียวกัน (STABLE_POPUP_PWM_RIPPLE)
  *** v20: PID Auto-Tune อัตโนมัติ (relay/Åström–Hägglund) + โปรไฟล์เกนแยกตามสภาพแวดล้อม ***
  - แผงจูน PID บนเว็บ (แอดมิน) เพิ่มปุ่ม "Auto-Tune อัตโนมัติ": สลับกำลังไฟเทลเทียร์สูง/ต่ำเอง วัดคาบ+แอมพลิจูดการแกว่ง
    คำนวณ Ku/Pu แล้วใช้สูตร Ziegler–Nichols หา Kp/Ki/Kd ให้อัตโนมัติ ใช้ได้เฉพาะตอนเครื่องว่างอยู่ที่เมนู (ไม่รบกวนรอบวัดจริง)
  - ผลที่จูนได้บันทึกลง NVS แยกเป็นโปรไฟล์ตามอุณหภูมิห้องตอนบูต (ปัดเป็นองศาเต็ม) บูตครั้งถัดไปในห้องใกล้เคียงเดิมจะ
    โหลดเกนที่เคยจูนไว้กลับมาใช้เองอัตโนมัติ ไม่ต้องจูนซ้ำทุกครั้งที่ย้ายเครื่อง; เอนด์พอยต์ /pidautotune, /pidautotune/status
  *** v21: "นิ่ง" ไม่จำกัดช่วงกว้าง aw อีกต่อไปเมื่อเป็น ripple จาก PWM + ค่าที่ล็อกใช้จุดกึ่งกลางการแกว่งแทนค่าเฉลี่ย ***
  - เดิม (v18/v19) แค่ผ่อนช่วงกว้าง aw ที่ยอมรับเป็นตัวคูณตอน PWM นิ่ง ยังมีเพดานอยู่ดี — ตอนนี้ถ้า PWM นิ่งตลอดหน้าต่าง
    (ตรวจเจอทุกบล็อก ไม่ใช่แค่บางครั้ง) ถือว่าค่าที่ขึ้น-ลงเป็น ripple ของลูปควบคุมล้วน ๆ นับว่า "นิ่ง" ได้โดยไม่จำกัด
    ช่วงกว้างอีกเลย (stabFlat()) ยังกันแนวโน้มไหลจริงที่อาจซ่อนอยู่ใต้ ripple ด้วยเกณฑ์ครึ่งแรก-ครึ่งหลัง + ความชัน 2 นาทีเดิม
  - ค่าที่ล็อกไว้ตอนไฟเขียว (measureFinalAw) และค่าเฉลี่ยที่ใช้ตั้ง/ตรวจจุดคาลิเบรตอัตโนมัติ (finishAutoCalRound) เปลี่ยนจาก
    ค่าเฉลี่ยเลขคณิตธรรมดา เป็นค่าเฉลี่ยของจุดสูงสุด-ต่ำสุดของการแกว่ง (mn+mx)/2 เมื่อ PWM นิ่งตลอดหน้าต่างนั้น (แม่นกว่า
    เพราะหน้าต่างล็อกมักไม่ครบพอดีจำนวนรอบ ripple เต็ม ๆ ซึ่งทำให้ค่าเฉลี่ยธรรมดาเอียงออกจากจุดกึ่งกลางจริงได้)
  *** v23: แก้บั๊ก "หน้าต่างตัดสินนิ่งแคบเกินคาบการแกว่งจริงของลูป PID" (ค้างสถานะ "ยังเคลื่อนที่" ไม่เคยนิ่งในคาลิเบตอัตโนมัติ) ***
  - จากข้อมูลจริง (กราฟ PWM/อุณหภูมิบนเว็บ) คาบการแกว่งของลูปควบคุมอยู่ที่ราว 110-120 วิ/คาบ แต่ STAB_WINDOW_MS (หน้าต่าง
    ตัดสิน "นิ่งสนิท") เดิมมีแค่ 60 วิ (ครึ่งคาบ) และ STAB_SLOPE_WINDOW_MS (หน้าต่างวัดความชัน) เดิมมีแค่ 120 วิ (~1 คาบ)
    ทำให้หน้าต่างมักจับได้แค่บางส่วนของรอบการแกว่งเดียว ไม่เคยเห็นภาพเต็มคาบที่แบนจริง — ขยับเป็น 4 นาที / 8 นาที ตามลำดับ
    (ครอบคลุมอย่างน้อย ~2 และ ~4 คาบเต็ม) พร้อมขยายบัฟเฟอร์ที่รองรับ (STAB_BLK_CAP, STAB_BUF_CAP, AUTOCAL_GRAPH_CAP,
    AUTOCAL_WIN_SAMPLES) ให้พอกับหน้าต่างใหม่ทุกตัว — เป็นบั๊กสายพันธุ์เดียวกับที่แก้ไปแล้วใน v12 (บัฟเฟอร์เล็กกว่าจำนวน
    ตัวอย่างที่ต้องใช้จริง) เพิ่ม static_assert คู่กันไว้ที่ท้ายไฟล์กันเหตุการณ์ซ้ำถ้ามีคนขยับค่าคงที่กลุ่มนี้อีกในอนาคต
  - ผลข้างเคียงที่ตั้งใจ: การวัดปกติ (ไม่ใช่คาลิเบตอัตโนมัติ) ก็ใช้ตัวตัดสินนิ่งชุดเดียวกัน (ตามเจตนาเดิมของ v16 ที่อยากให้
    "สีเดียวกัน = ความหมายเดียวกัน" ทุกโหมด) จึงกินเวลาก่อนขึ้นไฟเขียวนานขึ้นด้วย (จากขั้นต่ำ ๆ ~2 นาทีเป็น ~8 นาที เมื่อ
    ตัวอย่างมีลักษณะแกว่งตามลูป PID) หากไม่ต้องการผลกระทบนี้กับโหมดวัดปกติ ให้แยกค่าคงที่ชุดนี้ออกเป็นสองชุด (คาลิเบต
    อัตโนมัติ / วัดปกติ) แทนการใช้ร่วมกัน
  *** v24 (ชุด A): อ่านเซนเซอร์แม่นขึ้น ***
  - readRawAw(): อ่าน T และ RH เป็นคู่จากการวัดครั้งเดียว (readShtPair -> sht.readBoth()) แล้วใช้ T ของตัวอย่างที่ผ่านตัวกรองมาเฉลี่ย
    เป็น shtTempC (เดิมอ่าน T แยกอีกครั้งหลังเฉลี่ย RH) ตัวกรอง outlier เปลี่ยนจากเกณฑ์ตายตัว 3 %RH เป็น MAD (SHT_MAD_*)
  - SHT45 ใช้ไดรเวอร์ native และ readBoth ตรวจ CRC เอง — ไม่แตะ logic นิ่ง/ทำนาย/คาลิเบรตใด ๆ
  *** v25 (ชุด B): ชดเชยอุณหภูมิแม่นขึ้น ***
  - ดีดแบนด์ชดเชย Magnus ปรับตามข้อมูลจริง: gradientDeadbandC() = 0.15 + sd(offset) °C (ไม่เกิน 0.5) เมื่อ offset เรียนรู้ครบ; ไม่ครบใช้ 0.5 เดิม
  - เรียนรู้ offset SHT-DS18B20: น้ำหนักตัวอย่างใหม่ = max(0.3, 1/(n+1)), เก็บ sd แบบ exponentially-weighted ลง NVS ("sd"),
    เกณฑ์ทิ้งตัวอย่างแคบลงเหลือ 1.5 °C เมื่อเชื่อถือได้แล้ว; DS18B20 ตั้ง 12-bit ชัดเจน; /data มีฟิลด์ learnedOffsetSdC
  *** v26 (ชุด C): ตารางคาลิเบรตเรียบขึ้นโดยไม่แตะค่าที่ตั้งไว้ ***
  - calTableLookup(): ระหว่างโหนดใช้ PCHIP (เอกทิศ ไม่ overshoot) แบบจำกัดไม่ให้ต่างจากเส้นตรงเดิมเกิน CAL_PCHIP_MAX_DEV_AW (0.001 aw)
    ค่าที่โหนดทุกจุด, ตาราง factory/NVS/Quick Cal, ช่วงที่เป็นเส้นตรงอยู่แล้ว และค่านอกช่วงตาราง = เท่าเดิมทุกประการ; ตั้งเป็น 0 = พฤติกรรมเดิมล้วน
  *** v28: ตัวตรวจความนิ่งแบบ Bayesian (stabClassifyBlocks) ***
  - ทดสอบความชันด้วยขอบบนช่วงเชื่อมั่น |b|+1.645*sd <= เกณฑ์ โดยฟิตเส้นตรง+ripple แบบมีคาบ (สแกน 75-200 วิ) เลือกโมเดลด้วย BIC
    ผลจำลอง: ความชันจริงเท่าเกณฑ์พอดีผ่าน ~5% (เดิม ~54%), <= 0.0001 aw/นาที ผ่านทุกกรณี, >= 0.0003 ปฏิเสธทุกกรณี; STAB_BAYES_ENABLE 0 = เดิม
  - ยังไม่ได้ปรับตัวทำนายค่าสมดุล (Predict): จำลองแล้ว Bayesian เอ็กซ์โพเนนเชียลเดี่ยวมั่นใจเกินจริงเมื่อเส้นโค้งจริงมี 2 ชั้น จึงไม่นำมาแทน
  *** v27: พื้นผิวคาลิเบรต 2 มิติ (aw x อุณหภูมิ) ***
  - aw = ตาราง(raw) x gradientFactor() x surfaceFactor(aw, T) — ตาราง/ค่าที่ตั้งไว้ไม่ถูกแก้; ยังไม่บันทึกพื้นผิว = ตัวคูณ 1.0 = ผลเท่าเดิมเป๊ะ
  - สร้างจากผลคาลิเบตอัตโนมัติ (ช่อง 25/20/19 °C) เมื่อแอดมินสั่ง GET /admin/calsurface/commit (หลังครบ 10 รอบ); ดู /admin/calsurface/status; ล้าง /admin/calsurface/clear

  *** v29: Kalman/EKF/system-ID/MPC/TinyML + แก้ค่า aw จอเครื่อง/เว็บไม่ตรงกัน + กราฟกลับทิศ ***
  - คำนวณ aw ที่แสดงทางเดียว (awShownFromRaw): จอเครื่อง = เว็บ = ค่าที่ล็อก ; ลูปวัดบนเครื่องเป็นตัวเดินสถานะ (เว็บไม่ต้องโพลก็ได้ค่าเดียวกัน)
  - เริ่มวัดด้วยค่าดิบ (RH/100) ก่อน แล้วค่อยผสมไปหาค่าที่ปรับแล้ว (TO_START_HOLD_S / TO_START_RAMP_S) ; offset มี slew limit + กันทิศ (raw ขึ้น -> offset ห้ามลด)
  - โมดูล ADV (บล็อก "v29 ADV CORE" ด้านบน): Kalman 2 สถานะของ DS18B20 (ป้อน PID), EKF 5 สถานะของ aw (ชดเชย Magnus + ความชัน + delta),
    ระบุโมเดลเทลเทียร์ออนไลน์ + MPC ทดลอง (ถอยกลับ PID อัตโนมัติ), TinyML MLP 9-10-1 ค่าสมดุลรอง — ดู GET /adv/status , ตั้งค่า GET /adv/set?kf=&ifix=&mpc=
*/

#include <WiFi.h>
#include <WebServer.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
// SHT45 ใช้ไดรเวอร์ I2C แบบ native ด้านล่าง ไม่ใช้ Adafruit_SHT31
// v17: เซนเซอร์ความชื้นสำรอง DHT22/DHT11 — ใช้ไลบรารี "DHT sensor library" ของ Adafruit (ติดตั้งผ่าน Library
// Manager เพิ่มอีกตัวหนึ่ง ต้องมี "Adafruit Unified Sensor" ด้วยเป็น dependency — ทั้งคู่หาเจอในช่องค้นหา "DHT sensor library")
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <OneWire.h>
// สร้าง QR code บนจอ ใช้ไลบรารี qrcodegen ของ Project Nayuki (MIT license)
// ไม่ต้องติดตั้งผ่าน Library Manager — แค่โหลด 2 ไฟล์ qrcodegen.h และ qrcodegen.c
// จาก https://github.com/nayuki/QR-Code-generator (โฟลเดอร์ c/) มาวางไว้ในโฟลเดอร์เดียวกับไฟล์ .ino นี้
// Arduino IDE จะคอมไพล์ไฟล์ที่อยู่ในโฟลเดอร์สเก็ตช์ให้อัตโนมัติ ไม่มีปัญหาชนกับชื่อไฟล์ qrcode.h ของ ESP-IDF อีก
#include "qrcodegen.h" // ใช้ " " แทน < > เพราะเป็นไฟล์ที่วางไว้ในโฟลเดอร์สเก็ตช์ ไม่ใช่ไลบรารีที่ติดตั้งผ่าน Library Manager
#include <DallasTemperature.h>
// v-stability: Task watchdog ของ ESP32 เอง — ถ้าลูป loop() ค้างนานเกินกำหนด (เช่น I2C แขวนเพราะสายเซนเซอร์หลวม)
// จะรีเซ็ตบอร์ดอัตโนมัติแทนที่จะค้างเงียบ ๆ ไม่ตอบสนองอะไรเลย (เครื่องมือวัดเชิงพาณิชย์ทุกตัวมีกลไกนี้)
#include <esp_task_wdt.h>
#include <time.h> // v-pro: time_t/gmtime สำหรับนาฬิกา audit trail (nowEpoch()/formatEpoch())
#include <esp_system.h> // v13: esp_reset_reason() ใช้บันทึกสาเหตุรีเซ็ต (brownout/watchdog/panic)
#include <esp_wifi.h>   // v-dist: esp_wifi_ap_get_sta_list() อ่านค่าความแรงสัญญาณ (RSSI) ของอุปกรณ์ที่เชื่อมต่อ AP อยู่ ใช้ประมาณระยะห่าง

// ============================================================================
//  เลือกโหมดค่า aw — เป็นจุดเดียวที่ต่างกันระหว่างไฟล์ www.ino (คาลิเบรต) กับ www_raw.ino (ค่าดิบ)
//    AW_RAW_MODE 0 = CALIBRATED : ค่า aw ผ่านสมการคาลิเบรต piecewise-linear (applyCal() + ตาราง CAL_POINTS)
//                                 แก้จุดคาลิเบรตได้จากแผง "Calibration Points" บนเว็บ
//    AW_RAW_MODE 1 = RAW        : ค่า aw = %RH ดิบ/100 จากเซนเซอร์ตรง ๆ ไม่ผ่านคาลิเบรตเลย
//                                 (ซ่อนแผงคาลิเบรตบนเว็บ, ปิด /calset /calreset, ไม่โหลดจุดคาลิเบรตจาก NVS,
//                                  ไม่เตือน "คาลิเบรตเกินอายุ", ไฟล์ export ระบุ Value mode = RAW) ใช้เทียบเคียง/อ้างอิง
// ============================================================================
#define AW_RAW_MODE 0   // v14: 0 = คาลิเบรต (ใช้ตารางจุด + ชดเชยอุณหภูมิ) / 1 = ค่าดิบ RH/100 ตรง ๆ

// ============================================================================
//  v29 ADV CORE — Kalman / EKF / RLS system-ID / offset-free MPC / TinyML MLP
//  โค้ด C++ ล้วน (ใช้แค่ <math.h>, <string.h>) ไม่พึ่ง Arduino API -> ทดสอบบนคอมได้ แล้ววางลงสเก็ตช์ตรง ๆ
// ============================================================================

static inline bool  advOk(float v) { return v == v && fabsf(v) < 1e20f; }
static inline float advClamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
// ln(Psat/0.61094) ตามสูตร Magnus ตัวเดียวกับ psatKPa() ในสเก็ตช์เดิม และอนุพันธ์ตามอุณหภูมิ
static inline float advLnEs(float tC)  { return 17.625f * tC / (tC + 243.04f); }
static inline float advLnEsD(float tC) { float d = tC + 243.04f; return 17.625f * 243.04f / (d * d); }

// ---------------------------------------------------------------------------
// 1) Kalman Filter 2 สถานะ [T, dT/dt] — constant-velocity, dt เปลี่ยนได้, มี innovation gating
//    DS18B20 อัปเดต 1 Hz แต่ลูปหลักวิ่งเร็วกว่ามาก: Tat(dtSince) ให้ค่าต่อเนื่องระหว่างรอบวัด
// ---------------------------------------------------------------------------
struct AdvKF2 {
  float x[2];
  float P[2][2];
  float qAcc, r, gate2;
  bool  init;
  int   rejectStreak;
  float lastNis;
  void reset() { init = false; rejectStreak = 0; lastNis = 0; x[0] = x[1] = 0; P[0][0] = P[1][1] = 1; P[0][1] = P[1][0] = 0; }
  void config(float qa, float rv, float g2) { qAcc = qa; r = rv; gate2 = g2; reset(); }
  bool update(float z, float dt) {           // คืน true = ยอมรับค่าวัด
    if (!advOk(z)) return false;
    if (!(dt > 0.0f)) dt = 1.0f;
    if (dt > 5.0f) dt = 5.0f;
    if (!init) { x[0] = z; x[1] = 0; P[0][0] = r; P[0][1] = P[1][0] = 0; P[1][1] = 0.01f; init = true; return true; }
    float xp0 = x[0] + dt * x[1], xp1 = x[1];
    float dt2 = dt * dt, dt3 = dt2 * dt, dt4 = dt2 * dt2;
    float p00 = P[0][0] + dt * (P[0][1] + P[1][0]) + dt2 * P[1][1] + qAcc * dt4 * 0.25f;
    float p01 = P[0][1] + dt * P[1][1] + qAcc * dt3 * 0.5f;
    float p11 = P[1][1] + qAcc * dt2;
    float y = z - xp0, S = p00 + r;
    lastNis = y * y / S;
    if (lastNis > gate2 && rejectStreak < 3) {           // ค่ากระโดด: ข้าม (ติดกัน 3 ครั้งยอมรับ เหมือนตัวกรองเดิมของสเก็ตช์)
      rejectStreak++;
      x[0] = xp0; x[1] = xp1; P[0][0] = p00; P[0][1] = P[1][0] = p01; P[1][1] = p11;
      return false;
    }
    rejectStreak = 0;
    float k0 = p00 / S, k1 = p01 / S;
    x[0] = xp0 + k0 * y; x[1] = xp1 + k1 * y;
    P[0][0] = (1.0f - k0) * p00;
    P[0][1] = P[1][0] = (1.0f - k0) * p01;
    P[1][1] = p11 - k1 * p01;
    if (P[0][0] < 1e-9f) P[0][0] = 1e-9f;
    if (P[1][1] < 1e-9f) P[1][1] = 1e-9f;
    return true;
  }
  float Tat(float dtSince) const { if (dtSince < 0) dtSince = 0; if (dtSince > 3.0f) dtSince = 3.0f; return x[0] + x[1] * dtSince; }
  float T() const { return x[0]; }
  float Tdot() const { return x[1]; }
  float sdT() const { return sqrtf(P[0][0]); }
};

// ---------------------------------------------------------------------------
// 2) EKF 5 สถานะ สำหรับ aw:  x = [aw, aw_dot, Ts, Ts_dot, delta]   (delta = Tชิป - Tตัวอย่าง)
//    วัด: z1 = RH ที่ชิป (เศษส่วน, หลังตารางคาลิเบรตแล้ว)   h1 = aw * Psat(Ts)/Psat(Ts+delta)   <- ไม่เชิงเส้น (Magnus)
//         z2 = T ของชิป SHT   h2 = Ts + delta
//         z3 = T ของ DS18B20   h3 = Ts
//    อัปเดตทีละสเกลาร์ (ไม่ต้องกลับเมทริกซ์) -> เบา + เสถียร ; NIS gating ต่อการวัด
//    ผลลัพธ์: aw ที่ชดเชยอุณหภูมิแบบเหมาะสมที่สุดทางสถิติ + ความชัน aw/นาที ± sd + delta ที่เรียนรู้สด
// ---------------------------------------------------------------------------
struct AdvEKF5 {
  float x[5];
  float P[5][5];
  float qAw, qT, qDelta;        // ความแปรปรวนของ "ความเร่ง" aw / T (ต่อ s^2) และ random-walk ของ delta (ต่อ s)
  float rRh, rTc, rTs;          // ความแปรปรวนการวัด
  float gateRh, gateT;          // เกณฑ์ NIS
  bool  init;
  int   rejRh, rejTc, rejTs;
  float nisRh, nisTc, nisTs;
  uint32_t nRejTotal, nRejRh, nRejTc, nRejTs;
  void config() {
    qAw = 4e-14f; qT = 1e-6f; qDelta = 1e-4f;
    rRh = 4e-7f; rTc = 4e-4f; rTs = 1e-3f;
    gateRh = 16.0f; gateT = 16.0f;
    reset();
  }
  void reset() { init = false; rejRh = rejTc = rejTs = 0; nisRh = nisTc = nisTs = 0; nRejTotal = nRejRh = nRejTc = nRejTs = 0; memset(x, 0, sizeof(x)); memset(P, 0, sizeof(P)); }
  static float lnRatio(float ts, float tc) { return advLnEs(ts) - advLnEs(tc); }
  void initFrom(float rh, float tc, float ts) {
    x[0] = advClamp(rh * expf(-lnRatio(ts, tc)), 0.0f, 1.2f); x[1] = 0; x[2] = ts; x[3] = 0; x[4] = tc - ts;
    memset(P, 0, sizeof(P));
    P[0][0] = 1e-4f; P[1][1] = 1e-8f; P[2][2] = 4e-3f; P[3][3] = 1e-4f; P[4][4] = 0.09f;
    init = true;
  }
  void predict(float dt) {
    x[0] += dt * x[1]; x[2] += dt * x[3];
    float A[5][5];
    for (int j = 0; j < 5; j++) { A[0][j] = P[0][j] + dt * P[1][j]; A[1][j] = P[1][j]; A[2][j] = P[2][j] + dt * P[3][j]; A[3][j] = P[3][j]; A[4][j] = P[4][j]; }
    for (int i = 0; i < 5; i++) { P[i][0] = A[i][0] + dt * A[i][1]; P[i][1] = A[i][1]; P[i][2] = A[i][2] + dt * A[i][3]; P[i][3] = A[i][3]; P[i][4] = A[i][4]; }
    float dt2 = dt * dt, dt3 = dt2 * dt, dt4 = dt2 * dt2;
    P[0][0] += qAw * dt4 * 0.25f; P[0][1] += qAw * dt3 * 0.5f; P[1][0] += qAw * dt3 * 0.5f; P[1][1] += qAw * dt2;
    P[2][2] += qT * dt4 * 0.25f;  P[2][3] += qT * dt3 * 0.5f;  P[3][2] += qT * dt3 * 0.5f;  P[3][3] += qT * dt2;
    P[4][4] += qDelta * dt;
  }
  bool scalarUpdate(const float H[5], float innov, float R, float gate, int& rej, float& nisOut, uint32_t& cnt) {
    float PH[5]; float S = R;
    for (int i = 0; i < 5; i++) { float s = 0; for (int j = 0; j < 5; j++) s += P[i][j] * H[j]; PH[i] = s; S += H[i] * s; }
    nisOut = innov * innov / S;
    if (nisOut > gate && rej < 3) { rej++; nRejTotal++; cnt++; return false; }
    rej = 0;
    float K[5];
    for (int i = 0; i < 5; i++) { K[i] = PH[i] / S; x[i] += K[i] * innov; }
    for (int i = 0; i < 5; i++) for (int j = 0; j < 5; j++) P[i][j] -= K[i] * PH[j];
    for (int i = 0; i < 5; i++) { for (int j = i + 1; j < 5; j++) { float m = 0.5f * (P[i][j] + P[j][i]); P[i][j] = P[j][i] = m; } if (P[i][i] < 1e-12f) P[i][i] = 1e-12f; }
    return true;
  }
  // rhFrac: RH ที่ชิป (0-1) หลังตารางคาลิเบรต ; tc: T ชิป ; ts: T ตัวอย่าง (DS18B20) — ค่าไหน NAN = ข้ามการวัดนั้น
  void step(float rhFrac, float tc, float ts, float dt) {
    if (!(dt > 0.0f)) dt = 1.0f;
    if (dt > 10.0f) dt = 10.0f;
    if (!init) { if (advOk(rhFrac) && advOk(tc) && advOk(ts)) initFrom(rhFrac, tc, ts); return; }
    predict(dt);
    if (advOk(ts)) { float H[5] = {0, 0, 1, 0, 0}; scalarUpdate(H, ts - x[2], rTs, gateT, rejTs, nisTs, nRejTs); }
    if (advOk(tc)) { float H[5] = {0, 0, 1, 0, 1}; scalarUpdate(H, tc - (x[2] + x[4]), rTc, gateT, rejTc, nisTc, nRejTc); }
    if (advOk(rhFrac)) {
      float tcs = x[2] + x[4];
      float e = expf(advLnEs(x[2]) - advLnEs(tcs));
      float h = x[0] * e;
      float H[5] = {e, 0, h * (advLnEsD(x[2]) - advLnEsD(tcs)), 0, -h * advLnEsD(tcs)};
      scalarUpdate(H, rhFrac - h, rRh, gateRh, rejRh, nisRh, nRejRh);
    }
    x[0] = advClamp(x[0], -0.05f, 1.2f);
  }
  float aw() const { return x[0]; }
  float awPerMin() const { return x[1] * 60.0f; }
  float sdAw() const { return sqrtf(P[0][0]); }
  float sdAwPerMin() const { return sqrtf(P[1][1]) * 60.0f; }
  float delta() const { return x[4]; }
  float sdDelta() const { return sqrtf(P[4][4]); }
};

// ---------------------------------------------------------------------------
// 4) ระบุโมเดลเทลเทียร์แบบฟิตทั้งหน้าต่าง (batch prediction-error) — แทน RLS ขั้นเดียวที่เกนสถานะนิ่งไม่เสถียร
//    โมเดล: 2 โพลจริง (p1 ช้า, p2 เร็ว) + เวลาหน่วง d ก้าว + เกนสถานะนิ่ง g (°C ต่อ PWM เต็ม, ติดลบ = ทำความเย็น)
//      y[k+1] = a1 y[k] + a2 y[k-1] + b1 u[k-d]     a1 = p1+p2, a2 = -p1 p2, b1 = g (1-p1)(1-p2)  -> DC gain = g พอดี
//    ฟิตบน "ส่วนต่าง" dy vs du (ตัดอุณหภูมิห้อง/ค่าคงที่ทิ้ง) : สำหรับทุก (d,p1,p2) จำลองผ่านตัวกรอง unit-gain แล้วแก้ g ด้วย
//    least squares แบบปิด ; เลือกชุดที่ R^2 สูงสุด  ต้อง "ถูกกระตุ้น" (u เปลี่ยนมากพอ) ก่อนจึงฟิต/เชื่อถือ — ไม่งั้นคงโมเดลเดิมไว้
//    ค่าคงที่ c (ความร้อนรั่วจากห้อง) ประมาณด้วยตัวกรองอันดับ 1 -> ควบคุมแบบ offset-free
// ---------------------------------------------------------------------------
#define ADV_FIT_M 180
#define ADV_FIT_ND 7
#define ADV_HIST 16
struct AdvPlantFit {
  float yb[ADV_FIT_M], ub[ADV_FIT_M];   // [0] = ใหม่สุด ; ub[i] = u ที่ใช้ตลอดช่วงก่อนถึง y[i]
  float dyS[ADV_FIT_M], duS[ADV_FIT_M], sS[ADV_FIT_M];   // scratch (อยู่ใน struct กัน stack)
  int   n;
  float th[4];                          // a1,a2,b1,b2 (b2 = 0)
  int   best;                           // เวลาหน่วง (ก้าว)
  float gFit, p1Fit, p2Fit, r2Fit;
  bool  ok;                             // มีโมเดลที่ผ่านเกณฑ์แล้ว
  int   sinceFit;
  float cAbs; bool cInit; float Ld;
  uint32_t nFits, nAccepted;
  float lastWinR2;                      // R^2 ของโมเดลปัจจุบันบนหน้าต่างล่าสุด (ถ้าถูกกระตุ้น)
  void reset() {
    n = 0; memset(yb, 0, sizeof(yb)); memset(ub, 0, sizeof(ub));
    th[0] = th[1] = th[2] = th[3] = 0; best = 2; gFit = 0; p1Fit = p2Fit = 0; r2Fit = 0; ok = false; sinceFit = 0;
    cAbs = 0; cInit = false; Ld = 0.3f; nFits = nAccepted = 0; lastWinR2 = 0;
  }
  void push(float y, float uPrev) {
    memmove(yb + 1, yb, sizeof(float) * (ADV_FIT_M - 1)); memmove(ub + 1, ub, sizeof(float) * (ADV_FIT_M - 1));
    yb[0] = y; ub[0] = uPrev; if (n < ADV_FIT_M) n++;
    sinceFit++;
    if (ok && n > 10) {
      float c = yb[0] - (th[0] * yb[1] + th[1] * yb[2] + th[2] * ub[best]);
      if (advOk(c)) { if (!cInit) { cAbs = c; cInit = true; } else cAbs += Ld * (c - cAbs); }
    }
    if (n >= 100 && sinceFit >= 20) { sinceFit = 0; fitWindow(); }
  }
  // คืน R^2 ของ (d,p1,p2) ที่กำหนดบนหน้าต่างปัจจุบัน และ g (least squares)
  float evalCand(int d, float p1, float p2, int M, float& gOut, float tot, int B) {
    float a1 = p1 + p2, a2 = -p1 * p2, g0 = (1.0f - p1) * (1.0f - p2);
    float s1 = 0, s2 = 0, num = 0, den = 0;
    for (int k = 0; k < M; k++) {
      float uin = (k - d >= 0) ? duS[k - d] : 0.0f;
      float s = a1 * s1 + a2 * s2 + g0 * uin;
      s2 = s1; s1 = s; sS[k] = s;
      if (k >= B) { num += s * dyS[k]; den += s * s; }
    }
    if (den < 1e-9f) { gOut = 0; return -1.0f; }
    float g = num / den; gOut = g;
    float err = 0;
    for (int k = B; k < M; k++) { float e = dyS[k] - g * sS[k]; err += e * e; }
    return 1.0f - err / tot;
  }
  void fitWindow() {
    int M = n - 1;                                   // จำนวนส่วนต่าง
    for (int i = 0; i < M; i++) {                    // เรียงเก่า -> ใหม่
      int src = M - i;                               // yb[src] เก่ากว่า yb[src-1]
      dyS[i] = yb[src - 1] - yb[src];
      duS[i] = ub[src - 1] - ub[src];
    }
    int B = 30;
    float tot = 0, duAbs = 0;
    for (int k = B; k < M; k++) { tot += dyS[k] * dyS[k]; }
    for (int k = 0; k < M; k++) duAbs += fabsf(duS[k]);
    if (duAbs < 0.5f || tot < 1e-4f) { return; }     // ไม่ถูกกระตุ้น (u แทบไม่เปลี่ยน) -> ไม่ฟิต ไม่ตัดสิน
    nFits++;
    // ประเมินโมเดลปัจจุบันบนหน้าต่างใหม่ (ตรวจว่ายังใช้ได้)
    if (ok) { float g; lastWinR2 = evalCand(best, p1Fit, p2Fit, M, g, tot, B); (void)g; }
    static const float P1[6] = {0.80f, 0.86f, 0.90f, 0.93f, 0.95f, 0.97f};
    static const float P2[6] = {0.00f, 0.30f, 0.50f, 0.70f, 0.82f, 0.90f};
    float bestR2 = -1e9f, bg = 0, bp1 = 0, bp2 = 0; int bd = 0;
    for (int d = 0; d < ADV_FIT_ND; d++) for (int i = 0; i < 6; i++) for (int j = 0; j < 6; j++) {
      if (P2[j] > P1[i]) continue;
      float g; float r2 = evalCand(d, P1[i], P2[j], M, g, tot, B);
      if (g <= -0.3f && g >= -80.0f && r2 > bestR2) { bestR2 = r2; bg = g; bp1 = P1[i]; bp2 = P2[j]; bd = d; }
    }
    if (bestR2 >= 0.5f && (!ok || bestR2 >= r2Fit - 0.05f || lastWinR2 < 0.25f)) {   // ผ่านเกณฑ์ และไม่แย่ลงเมื่อเทียบของเดิม
      p1Fit = bp1; p2Fit = bp2; gFit = bg; best = bd; r2Fit = bestR2; ok = true; nAccepted++;
      th[0] = bp1 + bp2; th[1] = -bp1 * bp2; th[2] = bg * (1.0f - bp1) * (1.0f - bp2); th[3] = 0.0f;
      cInit = false;   // ความสัมพันธ์ c ผูกกับโมเดล -> เริ่มประเมินใหม่
    } else if (ok && lastWinR2 < 0.15f && bestR2 < 0.3f) {
      ok = false;      // ข้อมูลที่ถูกกระตุ้นล่าสุดอธิบายไม่ได้ทั้งโมเดลเก่าและใหม่ -> ถอนความเชื่อถือ
    }
  }
  const float* theta() const { return th; }
  float explained() const { return r2Fit; }
  float gainDC() const { return gFit; }
  bool  valid() const { return ok && gFit <= -0.3f; }
};

// ---------------------------------------------------------------------------
// 5) MPC แบบ offset-free (เทลเทียร์ทำความเย็นอย่างเดียว) — condensed QP + move-blocking, แก้ด้วย projected Gauss-Seidel
//    ตัวแปร: U[0..NU-1] = ระดับ PWM (0..1) ของแต่ละบล็อก ; ขอบเขต 0<=U<=1 และจำกัดอัตราเร่งของก้าวแรก
//    ต้นทุน: sum w_i*(T_i - Tref)^2 + rDu*sum (dU)^2 + rE*sum len*U^2   โดย w_i ใหญ่ขึ้น (negMult) เมื่อ "เย็นกว่าเป้า"
//    (เพราะแก้ไม่ได้ด้วยเทลเทียร์ทางเดียว -> ต้องไม่ไหลเลยเป้า)  ทำ 2 รอบ (IRLS) ให้ตัวถ่วงสอดคล้องกับวิถีที่ทำนาย
// ---------------------------------------------------------------------------
#define ADV_MPC_N  40
#define ADV_MPC_NU 5
#define ADV_MPC_PAD 8
struct AdvMpc {
  float q, rDu, rE, negMult, negMargin;
  int   blkLen[ADV_MPC_NU];
  int   sweeps;
  // ผลล่าสุด (ไว้แสดง/ดีบัก)
  float lastU[ADV_MPC_NU];
  float lastMinY, lastY1;
  void config() {
    q = 1.0f; rDu = 0.5f; rE = 0.0004f; negMult = 3.0f; negMargin = 0.03f; sweeps = 40;
    static const int bl[ADV_MPC_NU] = {1, 2, 4, 8, 25};
    for (int j = 0; j < ADV_MPC_NU; j++) blkLen[j] = bl[j];
    lastMinY = lastY1 = 0; for (int j = 0; j < ADV_MPC_NU; j++) lastU[j] = 0;
  }
  // จำลอง N ก้าว: y[i+1] = a1 y[i] + a2 y[i-1] + b1 u[i-d] + b2 u[i-d-1] + c   ; ub[PAD + t] = u ณ ก้าว t (t<0 = อดีต)
  static void simulate(const float* th, int d, float y0, float ym1, const float* ub, float c, float* out) {
    float ya = y0, yb = ym1;
    for (int i = 0; i < ADV_MPC_N; i++) {
      float yn = th[0] * ya + th[1] * yb + th[2] * ub[ADV_MPC_PAD + i - d] + th[3] * ub[ADV_MPC_PAD + i - d - 1] + c;
      out[i] = yn; yb = ya; ya = yn;
    }
  }
  // y0,ym1: ส่วนเบี่ยงเบนจากเป้า (T-Tref) ปัจจุบันและก้าวก่อน ; uhist[0]=u ก้าวก่อนหน้า ; cDev = ค่าคงที่ในพิกัดส่วนเบี่ยงเบน
  // คืน u0 (0..1)
  float solve(const float* th, int d, float y0, float ym1, const float* uhist, float cDev, float slewUp) {
    float ub[ADV_MPC_PAD + ADV_MPC_N];
    for (int t = 0; t < ADV_MPC_PAD; t++) ub[ADV_MPC_PAD - 1 - t] = uhist[t];   // ub[PAD-1] = u_{k-1}
    for (int t = 0; t < ADV_MPC_N; t++) ub[ADV_MPC_PAD + t] = 0.0f;
    float fr[ADV_MPC_N];
    simulate(th, d, y0, ym1, ub, cDev, fr);                                    // free response (u อนาคต = 0)
    // คอลัมน์การตอบสนองต่อ u=1 ของแต่ละบล็อก (จากสภาพศูนย์)
    float G[ADV_MPC_N][ADV_MPC_NU];
    float ubz[ADV_MPC_PAD + ADV_MPC_N], col[ADV_MPC_N];
    int start = 0;
    for (int j = 0; j < ADV_MPC_NU; j++) {
      for (int t = 0; t < ADV_MPC_PAD + ADV_MPC_N; t++) ubz[t] = 0.0f;
      for (int t = start; t < start + blkLen[j] && t < ADV_MPC_N; t++) ubz[ADV_MPC_PAD + t] = 1.0f;
      simulate(th, d, 0.0f, 0.0f, ubz, 0.0f, col);
      for (int i = 0; i < ADV_MPC_N; i++) G[i][j] = col[i];
      start += blkLen[j];
    }
    float U[ADV_MPC_NU]; float uprev = uhist[0];
    for (int j = 0; j < ADV_MPC_NU; j++) U[j] = uprev;
    float w[ADV_MPC_N];
    for (int i = 0; i < ADV_MPC_N; i++) w[i] = q;
    float lo[ADV_MPC_NU], hi[ADV_MPC_NU];
    for (int j = 0; j < ADV_MPC_NU; j++) { lo[j] = 0.0f; hi[j] = 1.0f; }
    hi[0] = advClamp(uprev + slewUp, 0.0f, 1.0f);
    for (int pass = 0; pass < 2; pass++) {
      float M[ADV_MPC_NU][ADV_MPC_NU], b[ADV_MPC_NU];
      for (int a = 0; a < ADV_MPC_NU; a++) {
        for (int c2 = 0; c2 < ADV_MPC_NU; c2++) { float s = 0; for (int i = 0; i < ADV_MPC_N; i++) s += w[i] * G[i][a] * G[i][c2]; M[a][c2] = s; }
        float s = 0; for (int i = 0; i < ADV_MPC_N; i++) s += w[i] * G[i][a] * fr[i];
        b[a] = s;
        M[a][a] += rDu * ((a < ADV_MPC_NU - 1) ? 2.0f : 1.0f) + rE * blkLen[a];
        if (a > 0) { M[a][a - 1] -= rDu; M[a - 1][a] -= rDu; }
      }
      b[0] -= rDu * uprev;
      for (int sw = 0; sw < sweeps; sw++) {
        for (int a = 0; a < ADV_MPC_NU; a++) {
          float g = b[a]; for (int c2 = 0; c2 < ADV_MPC_NU; c2++) g += M[a][c2] * U[c2];
          float un = U[a] - g / M[a][a];
          U[a] = advClamp(un, lo[a], hi[a]);
        }
      }
      // วิถีที่ทำนาย -> ปรับตัวถ่วงรอบถัดไป
      float mn = 1e9f;
      for (int i = 0; i < ADV_MPC_N; i++) {
        float yi = fr[i]; for (int j = 0; j < ADV_MPC_NU; j++) yi += G[i][j] * U[j];
        if (yi < mn) mn = yi;
        if (i == 0) lastY1 = yi;
        w[i] = q * (yi < -negMargin ? negMult : 1.0f);
      }
      lastMinY = mn;
    }
    for (int j = 0; j < ADV_MPC_NU; j++) lastU[j] = U[j];
    return U[0];
  }
};

// ---------------------------------------------------------------------------
// 6) TinyML: MLP 9-10-1 (ReLU) ทำนาย "ส่วนที่ยังต้องไหลอีก" ของค่าสมดุล  Δ = y_eq - y_last
//    อินพุต = ผลต่างของค่าบล็อก (ตัวอย่างละ 15 วิ) ย้อนหลัง 1,2,3,4,6,8,10,12 ก้าว เทียบค่าล่าสุด (หน่วย aw% / 2) + Δ ที่ AR เดิมทำนาย
//    น้ำหนักฝึกไว้ล่วงหน้า (train_tinyml.py) เก็บเป็น const ใน flash ~0.45 KB — ฝึกใหม่จากข้อมูลจริงของเครื่องได้ด้วยสคริปต์เดียวกัน
// ---------------------------------------------------------------------------
#define ADV_ML_NI 9
#define ADV_ML_NH 10
static const int ADV_ML_NLAG = 8;
static const int ADV_ML_LAGS[ADV_ML_NLAG] = {1, 2, 3, 4, 6, 8, 10, 12};
static const float ADV_ML_FSCALE = 50.0f;     // (ต่าง aw) x 50 = (ต่าง aw% ) / 2
// arEq = ค่าสมดุลที่สมการ AR เดิม (fitAR1/AR2 ผสมน้ำหนัก) ทำนายได้ ; ML ทำหน้าที่ "แก้ส่วนที่เหลือ" ของ AR (residual learning)
static inline bool advMlFeatures(const float* buf, int n, float arEq, float* x) {
  if (n < 7 || !advOk(arEq)) return false;
  float last = buf[n - 1];
  for (int i = 0; i < ADV_ML_NLAG; i++) {
    int idx = n - 1 - ADV_ML_LAGS[i]; if (idx < 0) idx = 0;
    x[i] = (last - buf[idx]) * ADV_ML_FSCALE;
  }
  x[ADV_ML_NLAG] = advClamp((arEq - last) * ADV_ML_FSCALE, -15.0f, 15.0f);
  return true;
}
struct AdvMlp {
  const float* W1;   // [NH][NI]
  const float* B1;   // [NH]
  const float* W2;   // [NH]
  float B2;
  void attach(const float* w1, const float* b1, const float* w2, float b2) { W1 = w1; B1 = b1; W2 = w2; B2 = b2; }
  // คืน Δ (หน่วย aw) = ค่าสมดุล - ค่าล่าสุด
  float infer(const float* x) const {
    float o = B2;
    for (int j = 0; j < ADV_ML_NH; j++) {
      float s = B1[j];
      for (int i = 0; i < ADV_ML_NI; i++) s += W1[j * ADV_ML_NI + i] * x[i];
      if (s > 0) o += W2[j] * s;
    }
    return o / ADV_ML_FSCALE;
  }
  float inferBase(const float* x) const { return infer(x); }
};

// สร้างโดย train_tinyml.py — synthetic only
// อย่าแก้มือ: รันสคริปต์ใหม่เพื่อฝึกซ้ำ
static const float ADV_MLP_W1[90] = { 1.1255978f, 0.7446564f, 0.1748132f, -0.6387444f, -0.8473346f, -0.7171232f, -0.1650310f, 0.3323539f, -0.2241630f, -0.2250699f, 0.2558215f, 0.1367809f, -0.0195823f, -0.8519074f, -0.5348343f, 0.0602420f, -0.6717557f, -0.5903909f, -0.8977860f, -0.6202932f, -0.9639635f, -0.4413567f, -1.1016239f, -0.1989038f, -0.1667744f, -0.3380500f, -1.1660928f, 0.7305673f, 0.7335754f, 0.5744694f, -0.4372916f, -0.4532020f, -0.5850106f, -0.2894826f, 0.8115922f, -0.3620380f, 0.0946056f, 0.6704708f, -0.0601389f, 0.1662311f, -0.1803516f, -0.4031189f, -0.3542643f, 0.4583836f, 0.6777776f, -0.8468374f, 0.4334379f, 0.1710332f, -0.0428325f, 1.0958197f, 0.2949427f, -0.1889937f, 0.2548705f, 0.1481769f, -0.0883456f, 0.2086253f, -0.1564738f, 0.0874551f, 0.5756645f, -0.2495817f, 0.0897181f, -0.2423945f, -0.2231101f, -0.8811287f, -0.7546425f, -0.4007557f, 0.3135608f, 1.2349821f, 0.2646683f, -0.0550156f, 0.7006347f, -0.4338894f, -0.2729280f, -0.1362261f, 0.5092281f, 0.1699493f, -0.2112761f, -0.2469391f, -0.2665431f, 0.6982310f, -0.3921568f, -0.0972337f, 0.1441293f, -0.1227256f, -0.1201003f, -0.3535627f, 0.1014636f, -0.3452998f, 0.4221196f, 0.4951016f };
static const float ADV_MLP_B1[10] = { 1.1905131f, -0.4426284f, 0.5541728f, -0.6059281f, 1.2724974f, 0.8862963f, 0.5087448f, -0.3359081f, 0.3010259f, -0.3490029f };
static const float ADV_MLP_W2[10] = { -0.5893886f, 0.5831211f, -0.3053113f, 0.8461791f, 0.5947663f, 0.4362500f, -0.4375280f, -0.2468140f, -0.6099511f, -0.6377329f };
static const float ADV_MLP_B2 = 0.1413746f;

struct AdvCfg { bool kf; bool ifix; bool mpc; };
static AdvCfg advCfg = { true, false, false };
static bool  advMpcOn = false;
static float advMpcU = 0.0f;


// ---------- สีที่ใช้วาดบนจอ TFT (ย้ายมาไว้บนสุด เพราะถูกใช้ตั้งแต่ช่วงต้นไฟล์) ----------
#define COL_BG TFT_BLACK
#define COL_GRID 0x39C7
#define COL_AXIS TFT_WHITE
#define COL_LINE 0x07FF
#define COL_TEMP TFT_ORANGE
#define COL_TEXT TFT_WHITE
#define COL_WARN TFT_RED
#define COL_OK TFT_GREEN
#define COL_TOUCH TFT_YELLOW
#define COL_COOKIE_BODY 0xD32C
#define COL_COOKIE_CHIP 0x4102
#define COL_FRUIT_BODY 0xFD20
#define COL_FRUIT_LEAF 0x07E0
#define COL_MEAT_BODY 0xF800
#define COL_MEAT_FAT 0xFFFF
#define COL_MILK_BODY 0x07FF

// ---------- ตั้งค่า Wi-Fi & Web Server ----------
const char* ssid = "AW_Meter";      // ชื่อ Wi-Fi ที่บอร์ดจะปล่อยออกมา
// Wi-Fi เป็นแบบมีรหัสผ่าน (WPA2) แต่หน้าเว็บแดชบอร์ดเองไม่ต้องล็อกอินซ้ำ (ใส่รหัส Wi-Fi ครั้งเดียวแล้วเข้าเว็บได้เลย)
// ต้องมีความยาวอย่างน้อย 8 ตัวอักษรตามข้อกำหนดของ WPA2 (ไม่งั้น WiFi.softAP จะตั้งรหัสไม่สำเร็จ)
// แก้รหัสผ่านที่ต้องการได้ตรงนี้ที่เดียว จะไปแสดงตรงกันทั้งหน้า "3.WiFi Info" บนจอ และในหน้าเว็บ/QR โดยอัตโนมัติ
char wifiPassword[13] = "aw12345678";

// v-dist: ค่าคงที่สำหรับ "ประมาณ" ระยะห่างจากความแรงสัญญาณ Wi-Fi (RSSI) ของอุปกรณ์ที่กำลังเชื่อมต่อ/ล็อกอินอยู่
// เป็นการประมาณคร่าว ๆ เท่านั้น (สูตร log-distance path loss) ไม่ใช่ระยะทางที่วัดแม่นยำ เพราะสัญญาณ Wi-Fi ถูกลด
// ทอนได้จากหลายปัจจัย (กำแพง คน โลหะ การสะท้อน) ปรับ 2 ค่านี้ให้เข้ากับสภาพแวดล้อมจริงได้
#define WIFI_RSSI_AT_1M -40    // dBm ที่วัดได้จริงเมื่ออุปกรณ์อยู่ห่างตัวเครื่อง 1 เมตร (ค่าเริ่มต้นทั่วไปของ ESP32)
#define WIFI_PATH_LOSS_N 2.5   // เลขชี้กำลังการลดทอนสัญญาณ: ~2 ที่โล่งไม่มีสิ่งกีดขวาง / ~3-4 ในอาคารที่มีผนัง-สิ่งกีดขวางมาก

WebServer server(80);

// ---------- สถานะสุขภาพระบบ (system fault flags) ----------
// ใช้แจ้งเตือนผู้ใช้แบบเห็นได้ชัดบนจอ/เว็บ เมื่อเซนเซอร์หลุด/อ่านค่าไม่ได้ หรือ Wi-Fi AP เปิดไม่สำเร็จ
// แทนที่จะปล่อยให้เครื่องทำงานต่อแบบเงียบ ๆ ทั้งที่ข้อมูลที่แสดงอาจไม่ใช่ค่าจริงอีกต่อไป (ค้างค่าเดิม)
bool sensorFaultSHT = false;       // true = อ่านค่าจาก SHT45 ไม่ได้เลยในรอบ oversample ล่าสุด
bool sensorFaultDS18B20 = false;   // true = อ่านค่าจาก DS18B20 ไม่ได้ (สายหลุด/ไม่พบอุปกรณ์บนบัส 1-Wire)
// v-fix: ค่า %RH ล่าสุดที่ handleData() อ่านจาก SHT ได้สำเร็จ ใช้แทนเมื่อรอบถัดไปอ่านไม่ได้ชั่วคราว (สายหลวม/
// I2C แฮงก์เป็นพักๆ) กันไม่ให้กราฟบนเว็บตกฮวบลงไปที่ 0.00 ทั้งที่ค่าจริงยังไม่ได้เปลี่ยน (ดู handleData ด้านล่าง)
float lastGoodRhWeb = 50.0;
bool wifiApFault = false;          // true = เปิด Wi-Fi Access Point ไม่สำเร็จตอนบูต
const uint8_t WDT_TIMEOUT_SEC = 10; // task watchdog: ถ้า loop() ไม่วนกลับมาภายใน 10 วิ ถือว่าค้าง -> รีบูต

// ---------- v13: ค่าปรับความเสถียร (แก้ตรงนี้ที่เดียว) ----------
// --- I2C (SHT35 + จอ LCD ใช้บัสเดียวกัน) ---
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;
const uint32_t I2C_CLOCK_HZ = 50000;        // 50 kHz: ทนสายยาว/สัญญาณรบกวนจาก PWM เทลเทียร์+Wi-Fi ได้ดีกว่า 100 kHz (ถ้าสายสั้นมากปรับเป็น 100000 ได้)
const uint16_t I2C_TIMEOUT_MS = 50;         // ถ้าอุปกรณ์ค้างเกินเวลานี้ให้เลิกรอ (กันลูปหลักค้างจนโดน watchdog)
const unsigned long LCD_REFRESH_MS = 2000;  // เขียนข้อความทั้ง 2 บรรทัดซ้ำทุกกี่ ms เพื่อ "ซ่อม" ตัวอักษรที่เพี้ยนจากสัญญาณรบกวน
const unsigned long LCD_REINIT_MS = 60000;  // สั่ง init จอใหม่ทุกกี่ ms (ซ่อมกรณีจอหลุดโหมด 4-bit แล้วขึ้นตัวอักษรมั่ว) 0 = ปิด
const unsigned long I2C_PROBE_MS = 5000;    // เช็คว่าจอ LCD ตอบ ACK ทุกกี่ ms (ไม่ตอบ 2 ครั้งติด -> ล้างบัส + init ใหม่)
const float TEMP_SPIKE_C = 3.0f;            // DS18B20: ค่าที่ต่างจากครั้งก่อนเกินนี้ใน 1 วิ ถือว่าเป็นค่ากระโดด (ข้ามไม่เกิน 3 ครั้งติด)
// --- Wi-Fi Access Point ---
const bool WIFI_AUTO_CHANNEL = true;        // สแกนตอนบูตแล้วเลือกช่อง 1/6/11 ที่มีคลื่นรบกวนน้อยสุด
const uint8_t WIFI_FALLBACK_CHANNEL = 6;    // ช่องที่ใช้ถ้าสแกนไม่ได้/ปิดสแกน
const uint8_t WIFI_MAX_CLIENTS = 3;         // จำนวนอุปกรณ์ที่ต่อได้พร้อมกัน (น้อยลง = เสถียรและกินกระแสน้อยลง)
const unsigned long WIFI_CHECK_MS = 10000UL;// ตรวจว่า AP ยังทำงานอยู่ทุกกี่ ms (ถ้าหลุดจะเปิดใหม่เอง)
// v22: ตามคำขอ "เพิ่มระยะทางของบอร์ดกับเครื่องที่เชื่อมต่อในเว็บ" — ปรับจาก 15 dBm เป็นกำลังสูงสุดของ ESP32 (19.5 dBm)
// ระยะที่ได้เพิ่มขึ้นราว 2-3 เท่าโดยประมาณ (ขึ้นกับสิ่งกีดขวาง/สัญญาณรบกวนจริงหน้างาน) แลกกับกระแสพีคตอนส่งสัญญาณที่สูงขึ้น
// เล็กน้อย — ถ้าเจอปัญหาไฟตก/บอร์ดรีเซ็ตเองตอนเชื่อมต่อเว็บ ให้ลดกลับไปที่ WIFI_POWER_15dBm หรือ WIFI_POWER_17dBm
wifi_power_t wifiTxPower = WIFI_POWER_19_5dBm;// กำลังส่งสูงสุด: ระยะไกลขึ้น (ค่าเดิม 15 dBm ประหยัดกระแสกว่าแต่ระยะสั้นกว่า)
// --- ชดเชยอุณหภูมิ (โหมดคาลิเบรตเท่านั้น) ---
// %RH ที่ SHT รายงาน = ความดันไอน้ำ / ความดันไออิ่มตัว "ที่อุณหภูมิของตัวชิป" แต่ aw ของตัวอย่างต้องเทียบกับความดันไออิ่มตัว
// "ที่อุณหภูมิของตัวอย่าง" — ถ้าตัวชิปกับตัวอย่างอุณหภูมิต่างกัน (ห้องแอร์เป่าลม / ตัวชิปอยู่ใกล้บอร์ดที่อุ่น) ค่า RH จะคลาด
// ประมาณ 6% ต่อ 1 °C  สูตร: aw = RH_ตารางคาลิเบรต x Psat(T_ชิป) / Psat(T_ตัวอย่าง)   T_ชิป = SHT.readTemperature(), T_ตัวอย่าง = DS18B20
const bool TEMP_GRADIENT_CORRECTION = true;   // false = ปิดการชดเชยนี้
// *** TODO (สอบเทียบจริง): ตอนนี้ยังเป็น 0.0 (ค่ายังไม่เคยตั้ง) ***
// วิธีหาค่าจริง: วางเครื่องทิ้งไว้เฉย ๆ (ไม่มีตัวอย่าง ไม่เปิดฮีตเตอร์/เทลเทียร์) ในห้องอุณหภูมิคงที่ ~30 นาที
// แล้วเข้าเมนู "4.System Health" บนจอเครื่อง อ่านค่าแถว "SHT-sample dT" (= T_SHT - T_DS18B20 ขณะนิ่งจริง)
// เอาตัวเลขนั้นมาใส่แทน 0.0f ด้านล่าง — ค่านี้คือ "offset เซนเซอร์เปล่า ๆ" ที่ไม่ควรถูกนับเป็น gradient จากห้องแอร์
float SHT_MINUS_DS_OFFSET_C = 0.0f;           // ส่วนต่างที่ถือว่า "เป็นแค่ offset ของเซนเซอร์สองตัว" (T_SHT - T_DS18B20 ตอนวางทิ้งไว้ในอากาศนิ่ง
                                              // เครื่องปิดฮีตเตอร์/เทลเทียร์นาน ~30 นาที) — ตั้งค่านี้แล้วจะไม่ถูกนับเป็นความต่างจริง
// v25 (ชุด B): GRADIENT_DEADBAND_C เป็น "เพดาน" ของดีดแบนด์ตอนที่ offset ยังไม่น่าเชื่อถือ (ยังเรียนรู้ไม่ครบ) เท่านั้น
// เมื่อ offset เรียนรู้ครบแล้ว ดีดแบนด์จริงคำนวณจาก gradientDeadbandC() = GRADIENT_DEADBAND_MIN_C + K x (ค่าเบี่ยงเบนมาตรฐานของ offset ที่วัดได้)
// (ที่ 25 °C ต่าง 1 °C ~ 6 %RH: ดีดแบนด์ 0.5 °C = ไม่ชดเชยราว 3 %RH ≈ 0.03 aw จึงต้องเล็กที่สุดเท่าที่ข้อมูลรองรับ)
const float GRADIENT_DEADBAND_MIN_C = 0.15f;  // ดีดแบนด์ต่ำสุดแม้ offset นิ่งมาก (ความละเอียด/ความคลาดสุ่มของเซนเซอร์สองตัว)
const float OFFSET_DB_K = 1.0f;               // ดีดแบนด์ = MIN + K x sd ของ offset
const float OFFSET_SD_SEED_C = 0.30f;         // sd ตั้งต้นของตัวอย่างแรก (ยังไม่รู้การกระจายจริง -> ประเมินแบบระวัง)
const float OFFSET_LEARN_MAX_JUMP_TRUSTED_C = 1.5f; // เมื่อ offset เชื่อถือได้แล้ว ตัวอย่างที่ต่างเกินนี้ถือว่าห้องไม่นิ่ง ทิ้ง (เดิม 3.0 ใช้ตอนยังไม่เชื่อถือ)
const float GRADIENT_DEADBAND_C = 0.5f;       // ความต่างไม่เกินนี้ไม่ชดเชย (อยู่ในความคลาดเคลื่อนของเซนเซอร์สองตัวรวมกัน) ส่วนที่เกินชดเชยแบบต่อเนื่อง

// ---------- Phase 1 (calibration roadmap): จดจำความชื้น/อุณหภูมิ "ห้องเปล่า" ตอนเปิดเครื่อง ----------
// แนวคิด: ทุกครั้งที่บูต ก่อนเปิดฮีตเตอร์ใด ๆ เลย ให้อ่านค่าห้องจริง (ไม่มีตัวอย่าง) เก็บไว้เป็น "จุดอ้างอิง"
// ของรอบทำงานนี้ ใช้เทียบ/ปรับจูน (Phase 2-3) ในอนาคต — รอบนี้แค่ "จำ" อย่างเดียว ยังไม่ไปยุ่งกับฮีตเตอร์/ค่าคาลิเบรตจริง
float ambientRHatBoot = NAN;         // %RH ห้องตอนบูตรอบนี้ (NAN = ยังไม่จับได้/เซนเซอร์มีปัญหา)
float roomHumidityNow = NAN;
bool roomEnvironmentChanged = false;
const float ROOM_ENV_CHANGE_RH_PCT = 5.0f; // ฝน/แอร์เปลี่ยนห้องเกินนี้ให้เตือน ไม่เอาไปเดาแก้ค่า aw
float ambientTempCatBoot = NAN;      // °C (SHT) ห้องตอนบูตรอบนี้
bool  ambientBootValid = false;      // true = ค่าตอนบูตรอบนี้จับสำเร็จจริง (ไม่ใช่ค่าค้างจาก NVS รอบก่อน)
time_t ambientBootEpoch = 0;         // เวลาที่จับ (0 = ยังไม่เคยซิงก์นาฬิกาตอนบูต ปกติในรอบแรกหลังเปิดเครื่อง)

// v-room-aw (2026-09-29): ค่าเป้าหมายเริ่มต้นที่ใช้แยก "AW ห้อง" ออกจากตัวอย่าง
// ห้องจะถูกคำนวณจาก baseline RH ที่จับได้ตอนบูต ไม่ได้บังคับให้เป็นค่าคงที่ตัวใดตัวหนึ่ง
// หมายเหตุ: เกลือแกง/น้ำเป็นค่าอ้างอิงมาตรฐานโดยประมาณที่ 25 °C ส่วนมาม่าขึ้นกับยี่ห้อ/สูตร
// จึงตั้ง 0.500 เป็นค่าเริ่มต้นและควรแทนด้วยค่าเฉลี่ยจากการวัดจริงของสินค้าที่ใช้งาน
const float AW_TARGET_TABLE_SALT       = 0.753f;  // saturated NaCl reference, ~25 °C
const float AW_TARGET_INSTANT_NOODLE   = 0.500f;  // typical noodle starting point; product-specific
const float AW_TARGET_CALCIUM_CHLORIDE = 0.290f;  // CaCl2 reference, approximate
const float AW_TARGET_WATER            = 1.000f;  // pure water upper reference
const float ROOM_AW_MATCH_TOL          = 0.030f;  // ระยะห่างที่ยังถือว่าเป็นสภาวะห้อง/ไม่ใช่ตัวอย่างใหม่

// ---------- Phase 2 (calibration roadmap): เรียนรู้ shtOffset (T_SHT - T_DS18B20 ตอนห้องนิ่ง) จากหลาย ๆ บูต ----------
// เดิม SHT_MINUS_DS_OFFSET_C เป็นค่าคงที่ที่ต้องตั้งมือ (ผู้ใช้วัดเองแล้วแก้ซอร์สโค้ด) - Phase 2 นี้เก็บตัวอย่าง
// จริงจากช่วง "ห้องนิ่ง ไม่มีฮีตเตอร์ทำงาน" ตอนบูตทุกครั้ง มาเฉลี่ยแบบ EMA (ถ่วงน้ำหนักไปทางค่าล่าสุด) แทน
// ยังคง fallback ไปใช้ SHT_MINUS_DS_OFFSET_C เดิมจนกว่าจะมีตัวอย่างมากพอที่จะเชื่อถือได้ (กันบูตแรก ๆ ที่ยังไม่นิ่งจริง)
const float OFFSET_LEARN_MAX_JUMP_C = 3.0f;      // ตัวอย่างที่ต่างจากค่าที่เรียนรู้ไว้เกินนี้ = ผิดปกติ (สัญญาณรบกวน/ห้องยังไม่นิ่ง) ทิ้งไป
const float OFFSET_LEARN_ALPHA = 0.3f;           // น้ำหนักของตัวอย่างใหม่ใน EMA (0.3 = ค่อย ๆ ปรับ ไม่กระโดดไปตามบูตเดียว)
const uint32_t OFFSET_LEARN_MIN_SAMPLES_TRUST = 3; // ต้องมีตัวอย่างที่ "ยอมรับ" อย่างน้อยเท่านี้ก่อนจะเริ่มใช้ค่าที่เรียนรู้แทนค่าคงที่เดิม
float learnedShtDsOffsetC = NAN;     // ค่า offset ที่เรียนรู้สะสมมา (NAN = ยังไม่มีข้อมูล)
uint32_t learnedOffsetSampleCount = 0; // จำนวนตัวอย่างที่ "ยอมรับ" สะสมมาแล้ว (ไม่นับตัวอย่างที่ถูกทิ้งเพราะกระโดดเกิน)
float learnedOffsetSdC = NAN;        // v25: ค่าเบี่ยงเบนมาตรฐานแบบถ่วงน้ำหนักเวลา (EW) ของตัวอย่าง offset — วัดความไม่แน่นอนของ offset จริง
bool learnedOffsetTrusted = false;   // true = มีตัวอย่างพอแล้ว ใช้ learnedShtDsOffsetC แทน SHT_MINUS_DS_OFFSET_C ได้
// v-cal-fix: เดิม 4.0°C — จากข้อมูลน้ำเปล่า (aw จริง≈1.00) ที่ raw ออกมาแค่ ~0.70 ซ้ำกัน 3 ครั้ง (0.704/0.708/0.712)
// ย้อนสูตร Magnus กลับ พบว่าต้องมีช่องว่างอุณหภูมิชิป-ตัวอย่างราว 5°C ถึงจะอธิบายค่าที่หายไปได้ ซึ่งเกินเพดานเดิม (4°C)
// การชดเชยจึงโดน clamp ไว้ไม่พอ ยิ่งห้องแอร์เย็น ช่องว่างนี้ยิ่งกว้าง (เทลเทียร์ไล่ตามห้องเย็นได้ แต่ชิป SHT ที่อยู่ใกล้
// บอร์ด/วงจรยังอุ่นตัวเองอยู่) — ขยับเพดานขึ้นชั่วคราวเป็น 8°C กันไม่ให้ตัด, แต่ "ทางแก้ที่ถูกจริง" คือลดความร้อนที่ตัวชิป
// (ย้าย SHT ให้ห่างวงจรร้อน/ลดโหลด I2C ระหว่างวัด) ไม่ใช่แค่ยอมให้สมการชดเชยเยอะขึ้นเรื่อย ๆ — ปรับตัวเลขนี้ให้ตรงกับ
// dT จริงสูงสุดที่วัดได้จากหน้า System Health อีกทีหลังเก็บข้อมูลรอบใหม่
const float GRADIENT_MAX_C = 8.0f;            // ชดเชยสูงสุดไม่เกินกี่ °C (กันกรณีเซนเซอร์ตัวใดตัวหนึ่งเพี้ยนจนแก้เกินจริง)
const float COLD_ROOM_MARGIN_C = 1.0f;        // ห้องเย็นกว่าเป้าหมายเกินนี้ (เทลเทียร์ทำความเย็นอย่างเดียว ทำความร้อนไม่ได้) = เตือน
// --- ไฟเลี้ยง / การบูต ---
const uint32_t CPU_FREQ_MHZ = 160;          // ลดจาก 240 MHz -> กินกระแสน้อยลงราว 30-40 mA (Wi-Fi ต้องการ >= 80 MHz)
const uint32_t SAFE_CPU_FREQ_MHZ = 80;
unsigned long peltierBootDelayMs = 8000UL;  // หน่วงไม่ให้เทลเทียร์/พัดลมทำงานนับจากตอน Wi-Fi ขึ้น (ให้ไฟเลี้ยงนิ่งก่อน)
float peltierSlewPerS = 100.0f;             // เร่งกำลังเทลเทียร์ได้ไม่เกินกี่ค่า PWM ต่อวินาที (255 ในราว 2.5 วิ) — ลดกระแสกระชากตอนเปิด
const int SAFE_START_AFTER_RESETS = 2;      // รีเซ็ตผิดปกติติดกันกี่ครั้งถึงเข้าโหมด SAFE START
const unsigned long BOOT_STABLE_CLEAR_MS = 120000UL; // ทำงานปกติเกินเวลานี้ = ล้างตัวนับรีเซ็ตติดกัน

// ---------- v-pro: นาฬิกา/audit trail/สถิติดริฟท์ (แทนที่ SD card ที่ตัดออกไปตามที่ผู้ใช้ระบุ) ----------
// ไม่มีชิป RTC สำรองแบตเตอรี่ต่อกับบอร์ด (เช่น DS3231) ดังนั้น "เวลาโลกจริง" ในเครื่องนี้มาจากการซิงก์
// ผ่านเว็บแดชบอร์ด (เบราว์เซอร์ส่งเวลาปัจจุบันมาให้ตอนเปิดหน้าเว็บ ดู handleClockSync()) เท่านั้น
// เมื่อไฟดับ/รีสตาร์ท เวลาจะ "ค้าง" อยู่ที่ค่าล่าสุดที่เคยซิงก์ไว้ใน NVS จนกว่าจะมีคนเปิดหน้าเว็บซิงก์ใหม่
// audit trail จึงมีธง clockVerified บอกตรง ๆ ว่าค่าที่บันทึกนี้ผูกกับเวลาที่ยืนยันแล้วหรือเป็นแค่ประมาณการ
// (นี่คือจุดที่ต่างจากเครื่องมือวัดระดับมืออาชีพที่มี RTC+แบตเตอรี่ในตัว — ถ้าต้องการความแม่นระดับนั้นจริง ๆ
//  แนะนำเพิ่มโมดูล DS3231 ต่อบัส I2C เดียวกับจอ LCD แล้วอ่านเวลาแทน millis() ในฟังก์ชัน nowEpoch() ด้านล่าง)
time_t clockEpochAtSync = 0;         // เวลา epoch (วินาที) ณ จังหวะที่ซิงก์ล่าสุด
unsigned long clockMillisAtSync = 0; // ค่า millis() ณ จังหวะเดียวกัน ใช้คำนวณเวลาปัจจุบันแบบต่อเนื่อง
bool clockEverSynced = false;        // true = เคยซิงก์เวลาอย่างน้อยหนึ่งครั้ง (จาก NVS หรือจากเว็บรอบนี้)
bool clockVerifiedThisBoot = false;  // true = ซิงก์เวลาจริงตั้งแต่บูตรอบนี้ (ไม่ใช่แค่ค่าค้างจาก NVS)

char operatorTag[9] = "";            // ชื่อย่อผู้ปฏิบัติงาน (ตั้งจากเว็บ) แนบไปกับทุกค่าที่บันทึกเพื่อ audit trail
time_t calSavedAtEpoch = 0;          // เวลาที่บันทึกจุดคาลิเบรตล่าสุด ใช้เตือน "คาลิเบรตเกินอายุ" บนหน้า System Health
const uint32_t CAL_REMINDER_DAYS = 90; // เตือนให้คาลิเบรตซ้ำทุก ~90 วัน (ตามแนวทางเครื่องมือวัดเชิงพาณิชย์ทั่วไป)

// v-pro: เฝ้าดูความนิ่ง (noise) ของค่าดิบระหว่างช่วง "stability window" ของการวัดแต่ละครั้ง — ถ้าเซนเซอร์
// เริ่มเสื่อม/สัมผัสหลวม มักจะอ่านค่ากระเพื่อมกว้างกว่าปกติแม้ตอนที่ระบบตัดสินว่า "STABLE" แล้วก็ตาม
// ต่างจาก isTempOutOfCalRange()/peltierStuckHot ที่เช็คช่วงอุณหภูมิ — อันนี้เช็คสถิติของค่าที่วัดได้เอง
bool lastMeasureNoiseWarning = false;   // true = การวัดครั้งล่าสุด ค่าดิบกระเพื่อมกว้างผิดปกติ (อาจเป็นสัญญาณดริฟท์/เซนเซอร์เสื่อม)
const float RAW_NOISE_WARN_RANGE = 0.006f; // ช่วงกว้างของค่าดิบ (เศษส่วน RH) ในหน้าต่างนิ่ง ที่เกินถือว่าน่าสงสัย

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="th"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>AW Meter Dashboard</title>
<style>
  :root{
    --bg:#0f1720; --bg-soft:#141f2b; --card:#182634; --card-2:#1e3040;
    --line:#28394a; --text:#eaf2f8; --muted:#8fa3b5;
    --aw:#38d9c9; --rh:#5aa9ff; --temp:#ff9f5a; --danger:#ff5d6c; --ok:#33d17a; --warn:#ffcc4d;
    --font-ui:'Segoe UI',Tahoma,Arial,sans-serif;
    --font-mono:ui-monospace,'SF Mono','Cascadia Mono',Consolas,'Courier New',monospace;
  }
  *{ box-sizing:border-box; }
  html,body{ margin:0; padding:0; }
  body{
    min-height:100vh; font-family:var(--font-ui); color:var(--text);
    background:radial-gradient(1200px 700px at 15% -10%, #1a3a44 0%, transparent 60%),
               radial-gradient(1000px 600px at 100% 0%, #22304a 0%, transparent 55%),
               var(--bg);
    padding:18px 14px 48px;
  }
  .wrap{ max-width:1180px; margin:0 auto; }
  a{ color:inherit; }

  /* ---------- Top bar ---------- */
  .topbar{
    display:flex; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:10px;
    margin-bottom:16px;
  }
  .brand{ display:flex; align-items:center; gap:10px; }
  .brand .logo{
    width:34px; height:34px; border-radius:9px; background:linear-gradient(155deg,var(--aw),#1c8f83);
    display:flex; align-items:center; justify-content:center; font-weight:800; color:#04201c; font-size:15px;
    flex:none;
  }
  .brand h1{ font-size:16px; margin:0; font-weight:700; letter-spacing:.2px; }
  .brand .sub{ font-size:11.5px; color:var(--muted); margin-top:1px; }
  .badge{
    font-size:10.5px; font-weight:700; letter-spacing:.4px; padding:3px 9px; border-radius:999px;
    background:rgba(56,217,201,.12); color:var(--aw); border:1px solid rgba(56,217,201,.35);
  }
  .conn{
    display:inline-flex; align-items:center; gap:7px; font-size:12.5px; color:var(--muted);
    background:var(--card); border:1px solid var(--line); padding:7px 13px; border-radius:999px;
  }
  .conn .dot{ width:8px; height:8px; border-radius:50%; background:var(--ok); box-shadow:0 0 0 3px rgba(51,209,122,.18); }
  .conn .dot.off{ background:var(--danger); box-shadow:0 0 0 3px rgba(255,93,108,.18); }
  .conn .dot.wait{ background:var(--warn); box-shadow:0 0 0 3px rgba(255,204,77,.18); }
  /* v-led-web: จุดสีสถานะ LED — สีตั้งด้วย inline style จาก JS (มีได้ถึง 8 สีตามผังสี LED จริงของเครื่อง จึงไม่ใช้ class ตายตัวแบบ .dot ปกติ) */
  .led-dot{ width:8px; height:8px; border-radius:50%; background:#555; box-shadow:0 0 0 3px rgba(255,255,255,.08); transition:background .15s ease; }
  .led-dot.blinking{ animation:pulse 1s ease-in-out infinite; }

  /* ---------- Banners ---------- */
  .banner{
    display:none; align-items:flex-start; gap:9px; font-size:13px; line-height:1.5;
    border-radius:12px; padding:11px 14px; margin-bottom:12px; border:1px solid transparent;
  }
  .banner.show{ display:flex; }
  .banner.fault{ background:rgba(255,93,108,.1); border-color:rgba(255,93,108,.35); color:#ffd3d7; }
  .banner.heater{ background:rgba(255,204,77,.1); border-color:rgba(255,204,77,.35); color:#ffe9ae; }
  .banner.info{ background:rgba(90,169,255,.1); border-color:rgba(90,169,255,.35); color:#cfe5ff; }

  /* ---------- Grid / cards ---------- */
  .grid{ display:grid; grid-template-columns:340px 1fr; gap:14px; align-items:start; }
  @media (max-width:860px){ .grid{ grid-template-columns:1fr; } }
  .card{
    background:linear-gradient(175deg,var(--card),var(--card-2)); border:1px solid var(--line);
    border-radius:16px; padding:16px; box-shadow:0 10px 28px rgba(0,0,0,.25);
  }
  .card h2{ font-size:13px; margin:0 0 12px; font-weight:700; letter-spacing:.2px; color:var(--text); }
  .card h2 small{ color:var(--muted); font-weight:500; }

  /* ---------- Gauge ---------- */
  .gauge-wrap{ position:relative; width:100%; max-width:250px; margin:0 auto; }
  .gauge{ width:100%; height:auto; display:block; }
  .gauge-track{ fill:none; stroke:#22323f; stroke-width:14; stroke-linecap:round; }
  .gauge-value{ fill:none; stroke:var(--aw); stroke-width:14; stroke-linecap:round; transition:stroke-dashoffset .4s ease, stroke .3s ease; }
  .gauge-readout{ position:absolute; left:50%; bottom:10%; transform:translateX(-50%); text-align:center; }
  .gauge-num{ display:block; font-family:var(--font-mono); font-weight:700; font-size:clamp(28px,7vw,38px); color:var(--aw); font-variant-numeric:tabular-nums; }
  .gauge-num.danger{ color:var(--danger); }
  .gauge-cap{ display:block; font-size:11px; color:var(--muted); margin-top:2px; }

  .mini-rows{ display:flex; flex-direction:column; gap:10px; margin-top:14px; }
  .mini{ background:rgba(255,255,255,.03); border:1px solid var(--line); border-radius:12px; padding:9px 12px; }
  .mini .mh{ display:flex; justify-content:space-between; align-items:baseline; font-size:11.5px; color:var(--muted); }
  .mini .mv{ font-family:var(--font-mono); font-weight:700; font-size:18px; margin-top:2px; }
  .mini .bar{ height:5px; border-radius:99px; background:#22323f; margin-top:7px; overflow:hidden; }
  .mini .bar i{ display:block; height:100%; width:0%; background:var(--rh); transition:width .35s ease; }
  .mini.temp .bar i{ background:var(--temp); }
  .unit{ font-size:11px; color:var(--muted); font-weight:500; margin-left:2px; }

  /* ---------- Recording controls ---------- */
  .rec-row{ display:flex; align-items:center; gap:10px; flex-wrap:wrap; margin-bottom:12px; }
  .rec-dot{ width:9px; height:9px; border-radius:50%; background:#3a4452; flex:none; }
  .rec-dot.on{ background:var(--danger); animation:pulse 1.4s ease-in-out infinite; }
  @keyframes pulse{ 0%,100%{ opacity:1; } 50%{ opacity:.35; } }
  .rec-status{ font-size:12.5px; color:var(--muted); flex:1 1 220px; }
  .rec-count{ font-family:var(--font-mono); color:var(--text); font-weight:700; }

  .btnrow{ display:flex; flex-wrap:wrap; gap:8px; }
  button{
    font-family:inherit; font-size:12.5px; font-weight:600; border-radius:10px; border:1px solid var(--line);
    padding:9px 14px; background:#1c2b39; color:var(--text); cursor:pointer; transition:filter .15s ease, transform .1s ease;
  }
  button:hover{ filter:brightness(1.15); }
  button:active{ transform:scale(.97); }
  button.primary{ background:linear-gradient(155deg,var(--aw),#1c8f83); color:#04201c; border-color:transparent; }
  button.stop{ background:linear-gradient(155deg,#ff7a7a,var(--danger)); color:#2c0508; border-color:transparent; }
  button.ghost{ background:transparent; }
  button:disabled{ opacity:.4; cursor:not-allowed; }

  .chart-box{ width:100%; overflow-x:auto; border-radius:12px; border:1px solid var(--line); background:#0e1822; }
  canvas{ display:block; width:100%; height:auto; }

  .readout-row{ display:grid; grid-template-columns:repeat(3,1fr); gap:8px; margin-top:12px; }
  .readout{ background:rgba(255,255,255,.03); border:1px solid var(--line); border-radius:12px; padding:9px 10px; text-align:center; }
  .readout .rl{ font-size:10.5px; color:var(--muted); }
  .readout .rv{ font-family:var(--font-mono); font-weight:700; font-size:16px; margin-top:2px; }

  .hint{ font-size:11.5px; color:var(--muted); line-height:1.5; margin-top:10px; }

  /* ---------- Runs list ---------- */
  .run-row{ display:flex; align-items:center; gap:9px; padding:8px 10px; border:1px solid var(--line); border-radius:10px; margin-bottom:7px; background:rgba(255,255,255,.02); flex-wrap:wrap; }
  .run-row .swatch{ width:11px; height:11px; border-radius:4px; flex:none; }
  .run-row .rname{ font-size:12.5px; font-weight:600; flex:1 1 130px; }
  .run-row input[type=number]{
    width:88px; font-family:var(--font-mono); font-size:12px; background:#0e1822; color:var(--text);
    border:1px solid var(--line); border-radius:8px; padding:5px 7px;
  }
  .run-row .del{ background:transparent; border:1px solid var(--line); color:var(--danger); padding:5px 9px; font-size:11px; }
  .run-row .rename{ background:transparent; border:1px solid var(--line); color:var(--text); padding:5px 9px; font-size:11px; }
  .run-row .predBadge{ font-size:11px; color:var(--muted); flex:none; }
  .empty-note{ font-size:12px; color:var(--muted); padding:8px 2px; }

  table{ width:100%; border-collapse:collapse; font-size:12px; }
  th,td{ padding:7px 8px; text-align:right; border-bottom:1px solid var(--line); white-space:nowrap; }
  th:first-child, td:first-child{ text-align:left; }
  th{ color:var(--muted); font-weight:600; font-size:11px; }
  .table-scroll{ overflow-x:auto; }

  /* ---------- Form rows ---------- */
  .frow{ display:flex; align-items:center; gap:8px; flex-wrap:wrap; margin-bottom:8px; }
  .frow label{ font-size:12px; color:var(--muted); min-width:120px; }
  .frow input[type=text]{
    flex:1 1 140px; font-family:var(--font-mono); font-size:12.5px; background:#0e1822; color:var(--text);
    border:1px solid var(--line); border-radius:8px; padding:7px 9px;
  }
  .kv{ display:flex; justify-content:space-between; font-size:12.5px; padding:6px 0; border-bottom:1px dashed var(--line); }
  .kv:last-child{ border-bottom:none; }
  .kv .k{ color:var(--muted); }

  /* v-fix: ป็อปอัปเล็กๆ เด้งบอกเมื่อค่า aw บนเว็บเริ่มนิ่ง (ดู stabBufWeb/updateStabilityPopup ใน JS) */
  .stable-toast{
    position:fixed; top:18px; right:18px; z-index:999;
    background:#0e2620; border:1px solid rgba(56,217,201,.45); color:var(--aw);
    font-family:var(--font-mono); font-weight:700; font-size:13px;
    padding:10px 16px; border-radius:10px; box-shadow:0 6px 18px rgba(0,0,0,.35);
    opacity:0; transform:translateY(-8px); pointer-events:none;
    transition:opacity .25s ease, transform .25s ease;
  }
  .stable-toast.show{ opacity:1; transform:translateY(0); }

  footer{ text-align:center; font-size:11.5px; color:var(--muted); margin-top:22px; }
</style>
</head><body>
<div class="wrap">
  <div class="stable-toast" id="stableToast">✓ ค่าคงที่แล้ว</div>

  <div class="topbar">
    <div class="brand">
      <div class="logo">aw</div>
      <div>
        <h1 id="deviceTitle">AW Meter Dashboard</h1>
        <div class="sub" id="deviceSub">Water Activity Research Console</div>
      </div>
      <span class="badge" id="modeBadge" style="display:none;">RAW</span>
      <span class="badge" id="roleBadge" style="display:none;"></span>
    </div>
    <div style="display:flex; align-items:center; gap:8px;">
      <!-- v-led-web: สีไฟสถานะ (LED) เดียวกับที่แสดงบนตัวเครื่องจริง อ่านจากฟิลด์ ledColor/ledBlink ใน /data -->
      <div class="conn" id="ledStatusWrap" title="สีไฟสถานะ (LED) เดียวกับที่แสดงบนตัวเครื่องจริงตอนนี้">
        <span class="dot led-dot" id="ledDot"></span><span id="ledStatusText">ไฟสถานะ: –</span>
      </div>
      <div class="conn"><span class="dot wait" id="dot"></span><span id="statusText">กำลังเชื่อมต่อ…</span></div>
      <button id="logoutBtn" class="ghost" title="ออกจากระบบ / สลับไปอีกบัญชีหนึ่ง (person / admin)" style="font-size:11.5px; padding:7px 12px;">ออกจากระบบ / สลับบัญชี</button>
    </div>
  </div>

  <div class="banner fault" id="faultBanner"></div>
  <div class="banner heater show" id="heaterBanner" style="display:none;">⚠️ ฮีตเตอร์ในตัวเซนเซอร์เปิดอยู่ (ระบบอัตโนมัติ) — ค่าที่อ่านได้ตอนนี้ไม่แม่นยำ รอฮีตเตอร์ปิดเองก่อนเริ่มวัดค่าจริง</div>
  <div class="banner heater" id="phaseBanner"></div>
  <div class="banner info" id="promptBanner"></div>

  <div class="grid">
    <!-- ---------- ซ้าย: เกจค่าปัจจุบัน ---------- -->
    <div class="card">
      <h2>ค่าปัจจุบัน <small>อัปเดตทุก 1 วินาที</small></h2>
      <div class="gauge-wrap">
        <svg class="gauge" viewBox="0 0 220 140">
          <path class="gauge-track" d="M20,120 A90,90 0 0 1 200,120"/>
          <path class="gauge-value" id="gaugeArc" d="M20,120 A90,90 0 0 1 200,120" stroke-dasharray="283" stroke-dashoffset="283"/>
        </svg>
        <div class="gauge-readout">
          <span class="gauge-num" id="gaugeNum">–.–––</span>
          <span class="gauge-cap">Water Activity (aw)</span>
        </div>
      </div>
      <div class="mini-rows">
        <div class="mini">
          <div class="mh"><span>ความชื้นสัมพัทธ์ (%RH)</span><span class="mv" id="vRh">–</span></div>
          <div class="bar"><i id="fillRh"></i></div>
        </div>
        <div class="mini temp">
          <div class="mh"><span>อุณหภูมิ</span><span class="mv" id="vTemp">–</span></div>
          <div class="bar"><i id="fillTemp"></i></div>
        </div>
      </div>
    </div>

    <!-- ---------- ขวา: บันทึกกราฟ ---------- -->
    <div class="card">
      <h2>กราฟแนวโน้ม (Aw &amp; อุณหภูมิ ตามเวลา)</h2>

      <div class="rec-row">
        <span class="rec-dot" id="recDot"></span>
        <span class="rec-status" id="recStatus">ยังไม่เริ่มบันทึก — ตัวเลขด้านบนจะอัปเดตสดตลอดเวลาอยู่แล้ว กดเริ่มบันทึกเพื่อเก็บข้อมูลเข้ากราฟ/CSV ได้ยาวไม่จำกัดเวลา จนกว่าจะพอ</span>
        <button class="primary" id="recToggleBtn">เริ่มบันทึกกราฟ</button>
      </div>
      <div class="hint" id="ctrlHint" style="margin-top:4px;">กด "เริ่มบันทึกกราฟ" = สั่งเครื่องเริ่มวัดจริงทันที (เหมือนกดปุ่มที่ตัวเครื่อง) ต้องล็อกอินก่อน</div>
      <div class="frow" style="margin-top:6px;"><label style="min-width:auto;"><input type="checkbox" id="yZoomToggle" checked style="margin-right:6px;">ซูมแกน Y ตามช่วงข้อมูลจริง (เห็นความต่างของค่าละเอียดขึ้น)</label></div>
      <div class="frow" id="rawToggleRow" style="display:none; margin-top:6px;">
        <label style="min-width:auto;"><input type="checkbox" id="rawToggle" checked style="margin-right:6px;">แอดมิน: แสดงกราฟค่าจริง (RAW) ซ้อนกับกราฟคาลิเบรต</label>
      </div>
      <!-- v-pro: โหมดวัดมืออาชีพ — ตอนนี้เลือกได้ทั้งแอดมินและผู้ใช้ทั่วไป เมื่อเปิดไว้ตอนกด "เริ่มบันทึกกราฟ" จะมีกราฟที่ 2
           (ทำนายค่าสมดุลแบบละเอียด) เด้งขึ้นควบคู่กับกราฟปกติทันที คำนวณจากข้อมูลชุดเดียวกันที่กำลังวัดอยู่
           ไม่ต้องไปวัดซ้ำที่เมนู "1.2 Predict" ต่างหากอีกรอบ ช่วยประหยัดเวลา — ตัวเครื่อง/LCD ยังเข้าหน้าตรวจปกติ
           (ST_MEASURE_AW) เหมือนกดปุ่มที่ตัวเครื่องทุกประการ ไม่มีอะไรเปลี่ยนฝั่งบอร์ด -->
      <div class="frow" id="proModeRow" style="display:none; margin-top:6px;">
        <label style="min-width:auto;"><input type="checkbox" id="proModeToggle" style="margin-right:6px;">โหมดวัดมืออาชีพ (เด้งกราฟทำนายคู่กับกราฟปกติ)</label>
      </div>

      <div class="chart-box"><canvas id="chart" width="1400" height="620"></canvas></div>

      <div class="readout-row">
        <div class="readout"><div class="rl">Aw</div><div class="rv" id="awVal">–</div></div>
        <div class="readout"><div class="rl">อุณหภูมิ</div><div class="rv" id="tempVal">–</div></div>
        <div class="readout"><div class="rl">เวลาที่บันทึก</div><div class="rv" id="timeVal">–</div></div>
      </div>

      <!-- v-formal-mode: เลือกว่ากราฟทางการ (PNG พื้นขาว, ใช้กับทั้งปุ่มนี้และปุ่ม "ดาวน์โหลดกราฟอ้างอิง" ในการ์ดออกใบรายงาน)
           จะพล็อตค่าไหน — ค่าคาลิเบรต (ค่าทางการที่รายงาน) / ค่าดิบก่อนคาลิเบรต / หรือซ้อนทั้งสองเส้นให้เทียบกัน -->
      <div class="frow" style="margin-top:6px;">
        <label for="formalValueModeSel">โหมดค่าในกราฟทางการ</label>
        <select id="formalValueModeSel" style="flex:1 1 220px; font-family:var(--font-mono); font-size:12.5px; background:#0e1822; color:var(--text); border:1px solid var(--line); border-radius:8px; padding:7px 9px;">
          <option value="aw">ค่าคาลิเบรต (aw) — ค่าทางการ</option>
          <option value="raw">ค่าดิบ (RAW ก่อนคาลิเบรต)</option>
          <option value="both">ซ้อนทั้งสองเส้น (คาลิเบรต + ดิบ)</option>
        </select>
      </div>
      <div class="hint" style="margin-top:2px;">ใช้กับทั้งปุ่ม "ดาวน์โหลดกราฟ PNG" ด้านล่างนี้ และปุ่ม "ดาวน์โหลดกราฟอ้างอิง" ในการ์ดออกใบรายงานผล — โหมด RAW/ซ้อนสองเส้น มีไว้เทียบเคียง/อ้างอิงเท่านั้น</div>

      <div class="btnrow" style="margin-top:12px;">
        <button id="resetBtn">รีเซ็ตกราฟปัจจุบัน</button>
        <button id="saveBtn">บันทึกไว้เทียบ</button>
        <button id="csvBtn">ดาวน์โหลด CSV</button>
        <button id="pngBtn" title="กราฟพื้นขาวแบบเอกสารทางการ พร้อมเลขที่กราฟ (Graph No.) ใช้แนบอ้างอิงกับใบรายงานผล">ดาวน์โหลดกราฟ PNG (พื้นขาว/ทางการ)</button>
      </div>

      <!-- v-pro: กราฟที่ 2 — ทำนายค่าสมดุลแบบละเอียด (เฉพาะตอนติ๊ก "โหมดวัดมืออาชีพ" ไว้) -->
      <div id="proPredictBox" style="display:none; margin-top:14px; border-top:1px solid var(--line); padding-top:12px;">
        <h3 style="margin:0 0 6px;">กราฟที่ 2: ทำนายค่าสมดุล (โหมดมืออาชีพ) <small>คำนวณสดจากข้อมูลชุดเดียวกับกราฟด้านบน</small></h3>
        <div class="chart-box"><canvas id="proPredictChart" width="1400" height="360"></canvas></div>
        <div class="hint" id="proPredictLegend">เส้นฟ้าทึบ = ค่าที่วัดได้จริง, เส้นประส้ม = เส้นโค้งที่คาดว่าจะไปถึง, เส้นประเขียว = ค่าสมดุลที่ทำนาย (เครื่องหมาย ~ = ยังไม่มั่นใจ ข้อมูลน้อยไป)</div>
        <div class="readout-row" style="margin-top:8px;">
          <div class="readout"><div class="rl">ค่าสมดุลที่ทำนาย</div><div class="rv" id="proPredEqVal">–</div></div>
          <div class="readout"><div class="rl">เวลาที่คาดว่าจะถึงสมดุล (ETA)</div><div class="rv" id="proPredEtaVal">–</div></div>
          <div class="readout"><div class="rl">ความมั่นใจ</div><div class="rv" id="proPredConfVal">–</div></div>
        </div>
        <div class="btnrow" style="margin-top:10px;">
          <button id="proPredictPngBtn" title="กราฟทำนายพื้นขาวแบบเอกสารทางการ บันทึกแยกจากกราฟปกติได้">ดาวน์โหลดกราฟทำนาย PNG (พื้นขาว/ทางการ)</button>
        </div>
      </div>
    </div>
  </div>

  <!-- ---------- แผงแอดมิน: กราฟ Offset ตามแนวโน้มค่าดิบ (v-trend-offset) ---------- -->
  <div class="card" id="offsetCard" style="margin-top:14px; display:none;">
    <h2>กราฟ Offset อัตโนมัติ (แอดมิน) <small>บอร์ดตัดสินใจจากแนวโน้มค่าดิบ — ดูว่าปรับกี่หน่วย ใช้สูตรอะไร ณ จุดนั้น</small></h2>
    <div class="frow">
      <label for="offRefInput">ค่าจริงที่รู้ (ไม่บังคับ)</label>
      <input type="text" id="offRefInput" placeholder="เช่น 0.7530 — ใส่แล้วจะมีเส้น error เทียบค่าจริง" style="max-width:260px;">
      <label style="min-width:auto;"><input type="checkbox" id="offZoomToggle" checked style="margin-right:6px;">ซูมแกน Y อัตโนมัติ</label>
    </div>
    <div class="chart-box" style="margin-top:8px;"><canvas id="offChart" width="1400" height="680"></canvas></div>
    <div class="hint" id="offLegend">เส้นฟ้า = aw หลังปรับ, เส้นเทา = aw จากตารางก่อนปรับ, เส้นเหลืองจุด = ค่าดิบ (raw), เส้นประส้ม = aw เป้าหมายของโซน ณ ขณะนั้น, เส้นตั้งสีชมพู = offset ที่ปรับ (ตาราง → หลังปรับ), เส้นตั้งสีเขียว = error ที่เหลือ (หลังปรับ → เป้าหมาย หรือ → ค่าจริงถ้ากรอกไว้), กราฟล่าง = offset ตามเวลา (แถบ ±0.10 = ขีดจำกัดโซนค่าต่ำ)</div>
    <div class="readout-row" style="margin-top:8px;">
      <div class="readout"><div class="rl">Offset ตอนนี้</div><div class="rv" id="offNowVal">–</div></div>
      <div class="readout"><div class="rl">โซน (z 0–3)</div><div class="rv" id="offZoneVal">–</div></div>
      <div class="readout"><div class="rl">ความชัน raw (ต่อนาที)</div><div class="rv" id="offSlopeVal">–</div></div>
      <div class="readout"><div class="rl">น้ำหนัก w</div><div class="rv" id="offWVal">–</div></div>
    </div>
    <div class="hint" id="offFormula" style="font-family:var(--font-mono);">สูตร: รอข้อมูล…</div>
    <div class="btnrow" style="margin-top:10px;">
      <button id="offResetBtn">ล้างกราฟ Offset</button>
      <button id="offCsvBtn">ดาวน์โหลด CSV Offset</button>
      <button id="offPngBtn" title="กราฟพื้นขาวแบบเอกสารทางการ พร้อมเลขที่กราฟ (Graph No.) ใช้แนบอ้างอิงกับใบรายงานผล">ดาวน์โหลดกราฟ Offset PNG (พื้นขาว/ทางการ)</button>
    </div>

    <!-- v-trend-offset: เส้นโค้งคาลิเบรตสด (raw -> aw หลังปรับ) พร้อมเส้นถดถอยเชิงเส้น + สมการ + R² แบบเดียวกับกราฟงานวิจัย -->
    <div style="margin-top:16px; border-top:1px solid var(--line); padding-top:12px;">
      <h3 style="margin:0 0 6px;">เส้นโค้งคาลิเบรตสด (raw → aw) <small>จุดดำ = ค่าที่วัดได้จริงเฉลี่ยต่อช่วง raw, เส้นแดง = สมการเส้นตรงที่ fit ได้</small></h3>
      <div class="frow">
        <label for="offCalZoneSel">ช่วงที่ใช้ fit</label>
        <select id="offCalZoneSel" style="max-width:220px; font-family:var(--font-mono); font-size:12.5px; background:#0e1822; color:var(--text); border:1px solid var(--line); border-radius:6px;">
          <option value="all">ทั้งหมด (ทุกโซน)</option>
          <option value="0">เฉพาะโซน 0 (CaCl2/MgCl2)</option>
          <option value="1">เฉพาะโซน 1 (MgCl2→NaCl)</option>
          <option value="2">เฉพาะโซน 2 (NaCl→KCl)</option>
          <option value="3">เฉพาะโซน 3 (KCl→น้ำบริสุทธิ์)</option>
        </select>
      </div>
      <div class="chart-box" style="margin-top:8px;"><canvas id="offCalCurve" width="900" height="420"></canvas></div>
      <div class="hint">แกน X = ค่าดิบ raw, แกน Y = aw หลังปรับ offset — ใช้ดูว่าจุดที่บอร์ดปรับให้จริงยังเรียงเป็นเส้นตรง (R² สูง) หรือกระจาย (ควรจูนค่าคงที่ TO_* ใหม่)</div>
      <div class="btnrow" style="margin-top:8px;">
        <button id="offCalPngBtn" title="กราฟพื้นขาวแบบเอกสารทางการของเส้นโค้งคาลิเบรตสด">ดาวน์โหลดเส้นโค้งคาลิเบรต PNG (พื้นขาว/ทางการ)</button>
      </div>
    </div>

    <!-- v-trend-offset: ตารางสรุปคุณภาพการปรับต่อโซน (R², N, ช่วง offset, error เฉลี่ย) แบบเดียวกับตาราง R²/LOD/LOQ ในงานวิจัย -->
    <div style="margin-top:16px; border-top:1px solid var(--line); padding-top:12px;">
      <h3 style="margin:0 0 6px;">สรุปคุณภาพการปรับต่อโซน <small>คำนวณสดจากข้อมูลที่เก็บไว้ด้านบน</small></h3>
      <div class="table-scroll">
        <table id="offStatsTable">
          <thead><tr><th>โซน</th><th>ช่วง raw ที่พบ</th><th>N (จุด)</th><th>R² (raw vs aw)</th><th>Offset ต่ำสุด</th><th>Offset สูงสุด</th><th>|error| เฉลี่ย</th></tr></thead>
          <tbody id="offStatsBody"><tr><td colspan="7" style="text-align:center; color:#8fa3b5;">ยังไม่มีข้อมูล</td></tr></tbody>
        </table>
      </div>
      <div class="hint">error คำนวณเทียบ "ค่าจริงที่รู้" ด้านบนถ้ากรอกไว้ ไม่งั้นเทียบกับเป้าหมายของโซนนั้น ๆ (target) — R² ต่ำในโซนไหนแปลว่าเส้นสมมติของโซนนั้นยังไม่ตรงกับตัวอย่างจริง ควรแก้ anchor/สโลปในโค้ด</div>
      <div class="btnrow" style="margin-top:8px;">
        <button id="offStatsPngBtn" title="ตารางสรุปพื้นขาวแบบเอกสารทางการ">ดาวน์โหลดตารางสรุป PNG (พื้นขาว/ทางการ)</button>
      </div>
    </div>
  </div>

  <!-- ---------- แผงแอดมิน: จูน PID เทลเทียร์ (ขั้นตอนที่ 1 — ทำก่อนคาลิเบรต แสดงเฉพาะล็อกอินเป็น admin) ---------- -->
  <div class="card" id="pidTuneCard" style="margin-top:14px; display:none;">
    <h2>ขั้นตอนที่ 1: จูน PID เทลเทียร์ <small>ปรับ Kp/Ki/Kd แล้วดูผลสดจนอุณหภูมิแกว่งน้อยที่สุด ก่อนเริ่มคาลิเบรต</small></h2>
    <div class="hint">
      วิธีจูนแบบเป็นขั้นเป็นตอน (บันทึกทุกครั้งที่กดใช้ค่าใหม่ ไว้ตอบกรรมการว่าตัวเลขแต่ละชุดมาจากการทดลองจริง ไม่ใช่เดา):
      1) ตั้ง Ki=0, Kd=0 แล้วเพิ่ม Kp ทีละน้อยจนกราฟเริ่มแกว่งคงที่ (ไม่ลู่เข้า ไม่ลู่ออก) — บันทึกค่า Kp นี้ไว้เป็น Ku และช่วงเวลาแกว่งครบรอบเป็น Pu (วินาที) จากกราฟด้านล่าง
      2) ตั้งต้นด้วยสูตร Ziegler–Nichols: Kp≈0.6×Ku, Ki≈1.2×Ku/Pu, Kd≈0.075×Ku×Pu แล้วกด "ใช้ค่านี้"
      3) ปรับละเอียดทีละตัว: ถ้ายังแกว่งเกิน ลด Kp หรือเพิ่ม Kd เล็กน้อย ถ้าขึ้นถึงเป้าหมายช้าไป เพิ่ม Ki เล็กน้อย ทำซ้ำจนกราฟเส้นอุณหภูมิ (สีฟ้า) แนบเส้นเป้าหมาย (เส้นประ) โดยแกว่งไม่เกิน ±0.2–0.3°C
      4) เมื่อพอใจแล้วค่อยไปขั้นตอนที่ 2 (คาลิเบรต) ด้านล่าง — อย่าคาลิเบรตก่อนจูน PID เสร็จ เพราะอุณหภูมิที่ยังแกว่งจะทำให้จุดคาลิเบรตคลาดเคลื่อน
      <br>หรือกด "Auto-Tune อัตโนมัติ" ด้านล่างให้เครื่องทำขั้นตอน 1-2 ให้เองแทน (สลับกำลังไฟสูง/ต่ำวัดคาบการแกว่งเอง แล้วคำนวณ Kp/Ki/Kd ด้วยสูตรเดียวกัน)
      — ใช้ได้เฉพาะตอนเครื่องว่างอยู่ที่เมนู (ยังไม่เริ่มวัด) และจะจำผลไว้แยกตามอุณหภูมิห้องขณะนั้น บูตครั้งถัดไปในห้องใกล้เคียงเดิมจะดึงค่านี้กลับมาใช้เองอัตโนมัติ ไม่ต้องจูนซ้ำ
    </div>
    <div class="frow" style="margin-top:10px; gap:10px; flex-wrap:wrap;">
      <label style="min-width:auto;">Kp <input type="number" id="pidKpIn" step="0.1" style="width:80px;"></label>
      <label style="min-width:auto;">Ki <input type="number" id="pidKiIn" step="0.01" style="width:80px;"></label>
      <label style="min-width:auto;">Kd <input type="number" id="pidKdIn" step="0.1" style="width:80px;"></label>
      <button class="primary" id="pidApplyBtn">ใช้ค่านี้</button>
      <span id="pidApplyMsg" class="hint" style="margin:0;"></span>
    </div>
    <div class="frow" style="margin-top:10px; gap:10px; flex-wrap:wrap;">
      <button class="primary" id="pidAutoTuneBtn">▶ เริ่ม Auto-Tune อัตโนมัติ</button>
      <button id="pidAutoTuneCancelBtn" style="display:none;">✕ ยกเลิก Auto-Tune</button>
      <span id="pidAutoTuneMsg" class="hint" style="margin:0;">ว่าง — ยังไม่ได้สั่งจูนอัตโนมัติ</span>
    </div>

    <div class="readout-row" style="margin-top:10px;">
      <div class="readout"><div class="rl">อุณหภูมิปัจจุบัน</div><div class="rv" id="pidTempVal">–</div></div>
      <div class="readout"><div class="rl">เป้าหมาย</div><div class="rv" id="pidTargetVal">–</div></div>
      <div class="readout"><div class="rl">Error</div><div class="rv" id="pidErrVal">–</div></div>
      <div class="readout"><div class="rl">กำลัง PWM</div><div class="rv" id="pidPwmVal">–</div></div>
      <div class="readout"><div class="rl">แกว่ง (peak-to-peak, 5 นาทีล่าสุด)</div><div class="rv" id="pidSwingVal">–</div></div>
    </div>

    <div class="chart-box" style="margin-top:10px;"><canvas id="pidChart" width="1400" height="420"></canvas></div>
    <div class="hint" id="pidChartLegend">เส้นฟ้า = อุณหภูมิจริง, เส้นประขาว = เป้าหมาย, แท่งส้มด้านล่าง = กำลัง PWM (%) — เก็บสดตลอดเวลาที่เปิดหน้านี้ค้างไว้ ไม่ต้องกด "เริ่มบันทึกกราฟ"</div>
    <div class="btnrow" style="margin-top:10px;">
      <button id="pidResetBtn">ล้างกราฟจูนนี้</button>
      <button id="pidCsvBtn">ดาวน์โหลด Log การจูน (CSV)</button>
    </div>
  </div>

  <!-- ---------- แผงแอดมิน: โหมดคาลิเบตอัตโนมัติ 5 รอบ (แสดงเฉพาะล็อกอินเป็น admin) ---------- -->
  <div class="card" id="adminCalCard" style="margin-top:14px; display:none;">
    <h2>โหมดคาลิเบตอัตโนมัติ (แอดมิน) <small>วัด 5 รอบ รอบละ 25 นาที — รอบ 1-3 ที่ 25°C, รอบ 4 ที่ 20°C, รอบ 5 ที่ 17°C</small></h2>
    <div class="frow" id="acalRefRow">
      <label for="acalRefInput">ค่าเป้าหมาย (aw อ้างอิงของสารละลายมาตรฐาน)</label>
      <input type="text" id="acalRefInput" placeholder="เช่น 0.7530 (0–1)" style="max-width:140px;">
    </div>
    <div class="hint" id="acalRefHint" style="color:#ffb300;">ต้องกรอกค่าเป้าหมาย (aw จริงของสารละลายมาตรฐานที่ใช้ทดสอบ) ในขั้นตอนแรกก่อนเริ่มเสมอ — ใช้เทียบผลทั้ง 5 รอบ ไม่งั้นคาลิเบตจะไม่ถูกต้อง</div>
    <div class="rec-row">
      <span class="rec-dot" id="acalDot"></span>
      <span class="rec-status" id="acalStatus">ยังไม่เริ่มโหมดคาลิเบตอัตโนมัติ</span>
      <button class="primary" id="acalStartBtn" disabled>เริ่มคาลิเบตอัตโนมัติ</button>
      <button class="stop" id="acalCancelBtn" style="display:none;">ยกเลิก</button>
    </div>
    <div class="hint">ระบบจะสั่งเทลเทียร์ลดอุณหภูมิไปที่เป้าหมายของแต่ละรอบเองก่อนเริ่มจับเวลา 25 นาที ไม่ต้องกดอะไรที่ตัวเครื่องระหว่างรอบ เครื่องต้องว่าง (อยู่หน้าเมนู) ก่อนเริ่ม — กราฟค่า aw สดของรอบที่กำลังวัดจะแสดงทั้งบนจอเครื่องและหน้าเว็บนี้ด้านล่าง</div>
    <div class="chart-box" style="margin-top:10px;"><canvas id="acalChart" width="1400" height="260"></canvas></div>
    <div class="hint" id="acalChartLegend">เส้นฟ้า = aw ที่วัดได้สด (คาลิเบรตแล้ว) ของรอบปัจจุบัน, เส้นประเหลือง = ค่าเป้าหมายที่กรอกไว้ — กราฟรีเซ็ตทุกครั้งที่ขึ้นรอบใหม่ (แสดงเหมือนกันบนจอเครื่องด้วย)</div>
    <div class="btnrow" style="margin-top:10px;">
      <button id="acalCsvBtn">ดาวน์โหลดกราฟคาลิเบต CSV</button>
      <button id="acalPngBtn" title="กราฟพื้นขาวแบบเอกสารทางการของรอบคาลิเบตปัจจุบัน">ดาวน์โหลดกราฟคาลิเบต PNG (พื้นขาว/ทางการ)</button>
    </div>

    <!-- v-pro: กราฟที่ 2 ของรอบคาลิเบต — ทำนายค่าสมดุลแบบละเอียดจากข้อมูลรอบปัจจุบัน (เด้งคู่กับกราฟด้านบนเสมอ
         ระหว่างที่รอบกำลังวัดอยู่ (ACAL_MEASURING) ให้เห็นแนวโน้มก่อนครบ 25 นาทีเต็ม ประหยัดเวลาไม่ต้องรอเดา) -->
    <div style="margin-top:14px; border-top:1px solid var(--line); padding-top:12px;">
      <h3 style="margin:0 0 6px;">กราฟที่ 2: ทำนายค่าสมดุลของรอบนี้ <small>คำนวณสดจากกราฟคาลิเบตด้านบน</small></h3>
      <div class="chart-box"><canvas id="acalPredictChart" width="1400" height="260"></canvas></div>
      <div class="hint" id="acalPredictLegend">เส้นฟ้าทึบ = ค่าที่วัดได้จริง, เส้นประส้ม = เส้นโค้งที่คาดว่าจะไปถึง, เส้นประเขียว = ค่าสมดุลที่ทำนาย, เส้นประเหลือง = เป้าหมาย</div>
      <div class="readout-row" style="margin-top:8px;">
        <div class="readout"><div class="rl">ค่าสมดุลที่ทำนาย</div><div class="rv" id="acalPredEqVal">–</div></div>
        <div class="readout"><div class="rl">ETA</div><div class="rv" id="acalPredEtaVal">–</div></div>
        <div class="readout"><div class="rl">ความมั่นใจ</div><div class="rv" id="acalPredConfVal">–</div></div>
      </div>
      <div class="btnrow" style="margin-top:10px;">
        <button id="acalPredictPngBtn" title="กราฟทำนายพื้นขาวแบบเอกสารทางการของรอบคาลิเบตนี้ บันทึกแยกจากกราฟปกติได้">ดาวน์โหลดกราฟทำนาย PNG (พื้นขาว/ทางการ)</button>
      </div>
    </div>
    <table style="margin-top:10px;">
      <thead><tr><th>รอบ</th><th>ประเภท</th><th>อุณหภูมิเป้าหมาย</th><th>สถานะ</th><th>RAW เฉลี่ย</th><th>aw เฉลี่ย</th><th>อุณหภูมิเฉลี่ย</th><th>%Error เทียบเป้าหมาย</th><th>ผลทดสอบ</th><th>เวลา</th></tr></thead>
      <tbody id="acalRoundsBody"></tbody>
    </table>
    <div class="btnrow" style="margin-top:10px;">
      <button class="primary" id="acalSaveBtn" style="display:none;">บันทึกผลเป็นจุดคาลิเบรต</button>
    </div>
    <div class="hint" id="acalSaveHint"></div>

    <!-- v-multi-sample: รายการตัวอย่างมาตรฐานหลายตัวที่คาลิเบตสะสมไว้ในเซสชันนี้ + ความเสถียร (SD) ของแต่ละตัวอย่าง -->
    <div style="margin-top:16px; border-top:1px solid var(--line); padding-top:12px;">
      <h3 style="margin:0 0 6px;">ตัวอย่างมาตรฐานที่คาลิเบตสะสมไว้ <small id="acalSampleCountLbl">(0 ตัวอย่าง)</small></h3>
      <div class="hint">แต่ละครั้งที่กด "บันทึกผลเป็นจุดคาลิเบรต" ด้านบน ระบบจะเพิ่ม 1 แถวที่นี่ พร้อมค่าเบี่ยงเบนมาตรฐาน (SD) ระหว่าง 3 รอบ 25°C ของตัวอย่างนั้น — ยิ่ง SD ต่ำยิ่งเสถียร ทำซ้ำหลายตัวอย่าง (สารละลายมาตรฐาน aw ต่างระดับ) เพื่อให้เส้นคาลิเบรตทั้งช่วงแม่นขึ้น</div>
      <table style="margin-top:8px;">
        <thead><tr><th>ตัวอย่างที่</th><th>เป้าหมาย aw</th><th>RAW เฉลี่ย</th><th>SD (ความเสถียร)</th><th>สถานะ</th></tr></thead>
        <tbody id="acalSamplesBody"><tr><td colspan="5" class="hint">ยังไม่มีตัวอย่างที่บันทึก</td></tr></tbody>
      </table>
      <div class="btnrow" style="margin-top:8px;">
        <button id="acalSamplesClearBtn">ล้างรายการตัวอย่างสะสม</button>
      </div>
    </div>
  </div>

  <!-- ---------- เปรียบเทียบกราฟที่บันทึกไว้ ---------- -->
  <div class="card" style="margin-top:14px;">
    <h2>เปรียบเทียบกราฟที่บันทึกไว้ <small>(ซ้อนหลายรอบทดสอบดูพร้อมกันได้)</small></h2>
    <div class="frow"><label style="min-width:auto;"><input type="checkbox" id="liveToggle" checked style="margin-right:6px;">แสดงกราฟปัจจุบัน (Live) ซ้อนด้วย</label></div>
    <div id="runList"></div>
    <div class="empty-note" id="emptyNote" style="display:none;">ยังไม่มีกราฟที่บันทึกไว้ — กด "บันทึกไว้เทียบ" ด้านบนเพื่อเก็บรอบทดสอบปัจจุบัน</div>
    <div class="hint">ใส่ "ค่าจริง (อ้างอิง)" ของแต่ละรอบ (เช่น ค่า aw ของสารละลายมาตรฐาน) เพื่อให้คำนวณ %error ให้อัตโนมัติในตารางด้านล่าง</div>

    <!-- v-pro: เปรียบเทียบกราฟ "ทำนายค่าสมดุล" ของรอบทดสอบที่บันทึกไว้ (ใช้ติ๊กแสดง/ซ่อนแถวเดียวกับด้านบน) -->
    <div id="comparePredictBox" style="display:none; margin-top:14px; border-top:1px solid var(--line); padding-top:12px;">
      <h3 style="margin:0 0 6px;">เปรียบเทียบกราฟทำนาย <small>(ค่าสมดุลที่ทำนายไว้ ณ ตอนบันทึกแต่ละรอบ — ติ๊กเลือกรอบด้านบนเพื่อซ้อนดู)</small></h3>
      <div class="chart-box"><canvas id="comparePredictChart" width="1400" height="320"></canvas></div>
      <div class="hint">เส้นทึบ = ช่วงที่มีข้อมูลจริงรองรับ, เส้นประ = ส่วนคาดการณ์ต่อไปจนถึงสมดุล (เครื่องหมาย ~ หน้าค่า = ยังไม่มั่นใจ ข้อมูลตอนวัดน้อยไป)</div>
      <button id="comparePredictPngBtn" style="margin-top:8px;" title="กราฟเปรียบเทียบค่าทำนายพื้นขาวแบบเอกสารทางการ ซ้อนทุกรอบที่ติ๊กแสดงอยู่ในไฟล์เดียว">ดาวน์โหลดกราฟเปรียบเทียบทำนาย PNG (พื้นขาว/ทางการ)</button>
    </div>
    <div class="btnrow" style="margin-top:10px;">
      <button id="clearAllBtn">ลบทั้งหมด</button>
      <button id="exportRunsBtn">ส่งออกข้อมูลดิบทั้งหมด (CSV)</button>
      <button id="exportStatsBtn">ส่งออกตารางสรุปผล (CSV)</button>
    </div>
    <div class="table-scroll" style="margin-top:12px;">
      <table id="statsTable" style="display:none;">
        <thead><tr><th>ชุดข้อมูล</th><th>ค่าจริง</th><th>ค่าดิบ</th><th>ค่าคาลิเบรตแล้ว</th><th>%Error (ดิบ)</th><th>%Error (คาลิเบรต)</th></tr></thead>
        <tbody id="statsBody"></tbody>
      </table>
    </div>
  </div>

  <!-- ---------- ออกใบรายงานผลการทดสอบ (Test Report / COA) ---------- -->
  <div class="card" style="margin-top:14px;">
    <h2>ออกใบรายงานผลการทดสอบ <small>(Test Report / Certificate of Analysis)</small></h2>
    <div class="frow">
      <label for="rptSource">ข้อมูลที่จะใช้ออกรายงาน</label>
      <select id="rptSource" style="flex:1 1 200px; font-family:var(--font-mono); font-size:12.5px; background:#0e1822; color:var(--text); border:1px solid var(--line); border-radius:8px; padding:7px 9px;">
        <option value="live">ค่าปัจจุบัน (Live)</option>
      </select>
    </div>
    <div class="frow"><label for="rptSampleName">ชื่อตัวอย่าง</label><input type="text" id="rptSampleName" placeholder="เช่น แป้งข้าวเหนียว ล็อต A"></div>
    <div class="frow"><label for="rptSampleId">รหัสตัวอย่าง / Lot No.</label><input type="text" id="rptSampleId" placeholder="เช่น LOT-2026-0912"></div>
    <div class="frow"><label for="rptCustomer">ลูกค้า / หน่วยงาน</label><input type="text" id="rptCustomer" placeholder="ไม่บังคับ"></div>
    <div class="frow"><label for="rptAnalyst">ผู้ทดสอบ (Analyst)</label><input type="text" id="rptAnalyst" placeholder="ชื่อผู้ทำการวัด"></div>
    <div class="frow"><label for="rptApprover">ผู้อนุมัติผล (Approved by)</label><input type="text" id="rptApprover" placeholder="ไม่บังคับ — เว้นว่างไว้เซ็นด้วยมือได้"></div>
    <div class="btnrow" style="margin-top:10px;">
      <button class="primary" id="reportBtn">ออกใบรายงานผล (PDF)</button>
      <button id="graphBtn">ดาวน์โหลดกราฟอ้างอิง (PNG พื้นขาว)</button>
    </div>
    <div class="hint">กดแล้วจะเปิดแท็บใหม่ที่จัดหน้าให้พร้อมพิมพ์ — ในแท็บนั้นกดปุ่ม "พิมพ์ / บันทึกเป็น PDF" แล้วเลือกปลายทางเป็น "บันทึกเป็น PDF" ในหน้าต่างพิมพ์ของเบราว์เซอร์ได้เลย<br>
      ใบรายงานไม่แนบกราฟ — ให้ดาวน์โหลดกราฟเป็นไฟล์ PNG พื้นขาวแยกต่างหากจากปุ่ม "ดาวน์โหลดกราฟอ้างอิง" (ใช้ชุดข้อมูล/ชื่อตัวอย่างเดียวกับที่เลือกไว้ด้านบน) ไฟล์กราฟมีเลขที่ AWG-… ตรงกับช่อง "กราฟอ้างอิง (Graph Ref.)" ในใบรายงาน<br>
      หมายเหตุ: ใบนี้เป็นข้อมูลเพื่อการตรวจสอบย้อนกลับ (traceability) เท่านั้น ไม่ใช่ใบรับรองที่ได้รับการรับรองมาตรฐาน ISO/IEC 17025 หรือ มผช.</div>
  </div>

  <div class="grid" style="margin-top:14px;">
    <!-- ---------- คาลิเบรต (แสดงเฉพาะเฟิร์มแวร์โหมดคาลิเบรต) ---------- -->
    <div class="card" id="calCard" style="display:none;">
      <h2>จุดคาลิเบรต <small>(piecewise-linear)</small></h2>
      <div id="calRows"></div>
      <!-- v-slope-live: ช่องใหม่ตามที่ขอ — โชว์สดว่าแต่ละช่วง raw→raw ถัดไป "ยืด/หด" สเกลเท่าไหร่ (สโลป) ทันทีที่พิมพ์เลข
           ไม่ต้องกดบันทึกก่อน ช่วยให้เห็นจุดผิดปกติตั้งแต่ตอนจูน (ปกติเซนเซอร์ SHT45 ควรใกล้ ×1 ทุกช่วง) รวมถึงโชว์ตัวคูณ
           ชดเชยอุณหภูมิที่ใช้งานจริงอยู่ตอนนี้ (gradientFactor) เพื่อดูภาพรวมความแม่นยำทั้งจูนปกติและจูนตามอุณหภูมิพร้อมกัน -->
      <div id="calSlopeBox" class="hint" style="margin-top:8px;"></div>
      <div class="btnrow" style="margin-top:10px;">
        <button class="primary" id="calSaveBtn">บันทึกจุดคาลิเบรต</button>
        <button id="calResetBtn">รีเซ็ตเป็นค่าโรงงาน</button>
        <button id="calSuggestBtn">เสนอจุดคาลิเบรตจากรอบที่บันทึกไว้</button>
      </div>
      <div class="hint" id="calHint"></div>
      <div id="calSuggest"></div>

      <!-- v22: คาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก — พิมพ์ค่าที่อ่านได้จากเครื่องคาลิเบรตแล้วกดใช้ทันที ไม่ต้องรอ AUTOCAL ครบ 10 รอบ -->
      <div style="margin-top:14px;border-top:1px solid rgba(255,255,255,.08);padding-top:10px;">
        <div style="font-weight:600;margin-bottom:4px;">คาลิเบรตแบบเร็ว (จากเครื่องอ้างอิงภายนอก)</div>
        <div class="frow">
          <label>ค่าอ้างอิง Aw ที่อ่านได้</label>
          <input type="text" id="quickCalRefAw" placeholder="เช่น 0.7530" style="max-width:130px;">
          <button class="primary" id="quickCalBtn">ใช้ค่านี้ทันที</button>
        </div>
        <div class="hint" id="quickCalHint">
          กด "ใช้ค่านี้ทันที" เมื่อตัวอย่างเดียวกันนิ่งแล้วทั้งสองเครื่อง — ระบบจะอ่านค่าดิบสดจากเครื่องนี้ ณ ขณะนั้น
          แล้วปรับเฉพาะจุดคาลิเบรตที่ใกล้เคียง โดยผสมกับตารางเดิมอยู่ (ไม่ล้างค่าคาลิเบรตก่อนหน้าทิ้ง) เร็วกว่าคาลิเบรตอัตโนมัติ 10 รอบมาก
        </div>
      </div>
    </div>

    <!-- ---------- สถานะระบบ & audit trail ---------- -->
    <div class="card" id="sysCard">
      <h2>สถานะระบบ &amp; Audit Trail</h2>
      <div class="frow">
        <label for="opInput">ผู้ปฏิบัติงาน</label>
        <input type="text" id="opInput" placeholder="ชื่อย่อ เช่น AB" maxlength="8">
        <button id="opSaveBtn">ตั้งค่า</button>
      </div>
      <div class="kv"><span class="k">นาฬิกา (audit)</span><span id="clockStat">–</span></div>
      <div class="kv"><span class="k">คาลิเบรตล่าสุด</span><span id="calAgeStat">–</span></div>
      <div class="kv"><span class="k">ความนิ่งของค่าล่าสุด</span><span id="noiseStat">–</span></div>
      <div class="kv"><span class="k">ผู้ปฏิบัติงานปัจจุบัน</span><span id="opStat">–</span></div>
      <div class="kv"><span class="k">อุณหภูมิ ชิป SHT − ตัวอย่าง</span><span id="dtStat">–</span></div>
      <div class="kv"><span class="k">ตัวคูณชดเชยอุณหภูมิ</span><span id="gfStat">–</span></div>
      <div class="kv"><span class="k">รุ่นเฟิร์มแวร์</span><span id="fwStat">–</span></div>
      <div class="kv"><span class="k">เซนเซอร์ความชื้น</span><span id="rhSensorStat">–</span></div>
      <div class="kv"><span class="k">เซนเซอร์อุณหภูมิ</span><span id="tempSensorStat">–</span></div>
      <!-- v-dist: ระยะห่างโดยประมาณระหว่างตัวเครื่องกับอุปกรณ์ที่กำลังเชื่อมต่อ/ล็อกอินดูหน้าเว็บนี้อยู่ (คำนวณจากความแรงสัญญาณ Wi-Fi) -->
      <div class="kv"><span class="k">ระยะห่างจากอุปกรณ์ที่ใช้งานอยู่ (ประมาณ)</span><span id="clientDistStat">–</span></div>
    </div>
  </div>

  <footer>อัปเดตทุก 1 วินาที เชื่อมต่อผ่าน Wi-Fi ของอุปกรณ์เอง</footer>
</div>

<script>
(function(){
  "use strict";

  // ============================================================
  //  สถานะกลาง
  // ============================================================
  var dataPoints = [];     // {t, aw, raw, temp, roomAw, roomRaw} ของรอบบันทึกปัจจุบัน
  var recording = false;   // ฝั่งเบราว์เซอร์ล้วนๆ ไม่ได้สั่งบอร์ด
  var missedBeats = 0;
  var lastData = null;
  var valueMode = 'CALIBRATED';
  var deviceInfoCache = null; // v-report: เก็บผลจาก /info ไว้ใช้ตอนออกใบรายงานผล กันไม่ต้องยิงคำขอซ้ำ
  var userRole = 'none';   // v-web-ctrl: 'admin' / 'person' / 'none' — ได้มาจาก GET /whoami หลังล็อกอินผ่าน (ดู loadRole())
  var measureStarting = false; // v-web-ctrl: กันกดปุ่ม "เริ่มบันทึกกราฟ" ซ้ำระหว่างรอเครื่องตอบรับคำสั่ง /cmd/measure
  var acalPollTimer = null;    // v-web-ctrl: ตัวจับเวลาโพลสถานะโหมดคาลิเบตอัตโนมัติ (เฉพาะแอดมิน)
  var acalRefAw = null;        // v-acal-graph: ค่าเป้าหมาย (aw อ้างอิง) ที่แอดมินกรอกไว้ก่อนเริ่ม — ต้องกรอกก่อนถึงจะกดเริ่มได้
  var acalGraphAw = [];        // v-acal-graph: จุดกราฟสด (aw คาลิเบรตแล้ว) ของรอบที่กำลังวัดอยู่ ได้จาก /admin/calmode/status
  // v-multi-sample: ตัวอย่างมาตรฐานที่คาลิเบตสะสมไว้ในเซสชันนี้ {ref, avgRaw, sd, savedAt} — 1 แถวต่อการกด "บันทึกผลเป็นจุดคาลิเบรต" ต่อสารละลายมาตรฐาน 1 ตัว
  var acalSamples = [];
  var ACAL_SD_STABLE_TOL = 0.01; // ถ้า SD ของ RAW เฉลี่ยระหว่าง 3 รอบ 25°C เกินนี้ ถือว่ายังไม่เสถียรพอ ควรวัดซ้ำสารละลายตัวนี้ใหม่

  // v-fix: ป็อปอัปแจ้งเตือนเมื่อค่า aw บนเว็บเริ่ม "นิ่ง" — ผู้ใช้ระบุว่าอยากให้เด้งตอนค่าแทบไม่ขยับเกิน
  // 0.001 ถึง 0.0005 จึงเลือกค่ากลาง (0.0008) เป็นเกณฑ์ "เข้าสู่ความนิ่ง" และค่าที่สูงกว่านั้นเล็กน้อยเป็นเกณฑ์
  // "ออกจากความนิ่ง" (hysteresis) กันป็อปอัปกระพริบถี่ๆ เวลาค่าแกว่งอยู่พอดีตรงขอบเขต ดูฟังก์ชัน
  // updateStabilityPopup() ด้านล่าง — ทำงานอิสระจากปุ่ม "เริ่มบันทึกกราฟ" คือเด้งได้ทุกครั้งที่เปิดหน้าเว็บดูค่าสด
  var stabBufWeb = [];              // {tMs, aw} หน้าต่างเวลาล่าสุดของค่า aw ที่อ่านได้ ใช้เช็คความนิ่งเท่านั้น
  var stablePopupShown = false;     // กันไม่ให้เด้งซ้ำรัวๆ ระหว่างที่ยังนิ่งอยู่ต่อเนื่อง
  var STABLE_POPUP_WINDOW_MS = 20000;
  var STABLE_POPUP_TOL = 0.0008;
  var STABLE_POPUP_EXIT_TOL = 0.0015;
  var STABLE_POPUP_PWM_RIPPLE = 0.0012; // v19-v20: เดิมใช้ผ่อนเกณฑ์ตอน PWM นิ่ง — v21: ไม่จำกัดช่วงกว้างอีกต่อไปตอน PWM นิ่ง (ดู updateStabilityPopup) เก็บไว้เผื่ออ้างอิง/ย้อนดูค่าเดิม
  var STABLE_POPUP_PWM_MULT = 2.5;   // v18-v20: เดิมใช้ผ่อนเกณฑ์ช่วงกว้าง aw ตอน PWM นิ่ง — v21: ไม่ได้ใช้คูณเกณฑ์แล้ว (ไม่จำกัดช่วงกว้างตอน PWM นิ่งเลย) เก็บไว้เผื่ออ้างอิง

  var STORAGE_KEY = 'aw_saved_runs_v2';
  var RUN_COLORS = ['#5aa9ff','#ff9f5a','#c084fc','#33d17a','#ff5d6c','#ffd166','#4dd0e1','#f472b6'];
  var savedRuns = loadRuns();

  function loadRuns(){
    try{
      var raw = localStorage.getItem(STORAGE_KEY);
      return raw ? JSON.parse(raw) : [];
    }catch(e){ return []; }
  }
  function persistRuns(){
    try{ localStorage.setItem(STORAGE_KEY, JSON.stringify(savedRuns)); }
    catch(e){ alert('บันทึกไม่สำเร็จ (พื้นที่จัดเก็บเต็มหรือถูกปิดใช้งาน)'); }
  }
  function nextColor(){ return RUN_COLORS[savedRuns.length % RUN_COLORS.length]; }

  function byId(id){ return document.getElementById(id); }

  // ============================================================
  //  ดึงข้อมูลสดจากบอร์ดทุก 1 วินาที
  // ============================================================
  function fetchWithTimeout(url, ms){
    var ctrl = new AbortController();
    var timer = setTimeout(function(){ ctrl.abort(); }, ms);
    return fetch(url, { signal: ctrl.signal }).finally(function(){ clearTimeout(timer); });
  }

  function setOnline(on, waiting){
    var dot = byId('dot'), txt = byId('statusText');
    if(!dot || !txt) return;
    dot.classList.remove('off','wait');
    if(waiting){ dot.classList.add('wait'); txt.textContent = 'กำลังเชื่อมต่อ…'; }
    else if(on){ txt.textContent = 'เชื่อมต่ออยู่'; }
    else { dot.classList.add('off'); txt.textContent = 'ขาดการเชื่อมต่อ — ลองตรวจสอบ Wi-Fi'; }
  }

  var pollInFlight = false; // v-fix: กันไม่ให้มีคำขอ /data ซ้อนกันมากกว่า 1 ชุดพร้อมกัน (ดูคำอธิบายด้านล่างใน poll())

  function poll(){
    if (pollInFlight) return; // ยังมีคำขอค้างอยู่ ข้ามรอบนี้ไปก่อน (จะยิงใหม่จากตัวจับเวลาที่ finally ตั้งไว้อยู่แล้ว)
    pollInFlight = true;
    var failed = false;
    fetchWithTimeout('/data', 2500).then(function(r){ return r.json(); }).then(function(data){
      missedBeats = 0;
      lastData = data;
      setOnline(true, false);

      try{ updateReadouts(data); }catch(e){ console.error('poll: อัปเดตตัวเลขหลักพลาด', e); }
      try{ updateLedStatus(data); }catch(e){ console.error('poll: อัปเดตไฟสถานะ LED พลาด', e); }
      try{ updateClientDistance(data); }catch(e){ console.error('poll: อัปเดตระยะห่างพลาด', e); }
      try{ updateStabilityPopup(data); }catch(e){ console.error('poll: เช็คความนิ่งพลาด', e); }
      try{ updateFaultBanner(data); }catch(e){ console.error('poll: อัปเดตแบนเนอร์พลาด', e); }
      try{
        // v12: ช่วงฮีต/รอเย็นก่อน-หลังวัดใช้แบนเนอร์เฟสของตัวเอง (updatePhaseBanner) แทนแบนเนอร์ฮีตเตอร์ทั่วไป
        byId('heaterBanner').style.display = (data.heater && !isHeatPhase(data)) ? 'flex' : 'none';
        updatePhaseBanner(data);
      }catch(e){ console.error('poll: อัปเดตแบนเนอร์ฮีตเตอร์พลาด', e); }
      try{ updateAuditPanel(data); }catch(e){ console.error('poll: อัปเดตแผง audit พลาด', e); }
      try{ if(userRole === 'admin') updatePidTuner(data); }catch(e){ console.error('poll: อัปเดตแผงจูน PID พลาด', e); }
      try{
        // v12: ไม่บันทึกจุดข้อมูลลงกราฟ/CSV ขณะเซนเซอร์กำลังฮีตหรือกำลังเย็นลง (ค่า %RH ช่วงนี้ไม่ใช่ค่าของตัวอย่าง)
        if(recording && !isHeatPhase(data) && !data.heater){
          if(!dataPoints.length){ liveDataId = null; liveStartedIso = new Date().toISOString(); } // จุดแรกของชุดข้อมูลใหม่ = เริ่ม Data ID ใหม่
          // v-fix: เมื่อบอร์ดแจ้ง faultSHT (อ่านเซนเซอร์ความชื้นไม่ได้ชั่วขณะ เช่น เว็บหลุดจากตัวเครื่องแวบเดียว)
          // หรือค่า aw/raw ที่ได้มาไม่ใช่ตัวเลขปกติ ให้ใช้จุดข้อมูลก่อนหน้าซ้ำแทนที่จะพล็อตค่าที่หลุดไป (เช่น 0.00)
          // เพื่อไม่ให้กราฟที่บันทึกมีเส้นดิ่งตกฮวบผิดปกติ
          var prevPt = dataPoints.length ? dataPoints[dataPoints.length - 1] : null;
          var awOk = (typeof data.aw === 'number') && !isNaN(data.aw) && !data.faultSHT;
          var newPt = {
            t: data.t,
            aw: awOk ? data.aw : (prevPt ? prevPt.aw : data.aw),
            raw: awOk ? data.raw : (prevPt ? prevPt.raw : data.raw),
            temp: (typeof data.temp === 'number' && !isNaN(data.temp)) ? data.temp : (prevPt ? prevPt.temp : data.temp),
            roomAw: (typeof data.roomAw === 'number' && data.roomAw >= 0 && data.roomAw <= 1) ? data.roomAw : (prevPt ? prevPt.roomAw : null),
            roomRaw: (typeof data.roomRaw === 'number' && data.roomRaw >= 0 && data.roomRaw <= 1) ? data.roomRaw : (prevPt ? prevPt.roomRaw : null),
            gf: (typeof data.gf === 'number' && !isNaN(data.gf)) ? data.gf : 1     // v14: ตัวคูณชดเชยอุณหภูมิ ณ จุดนี้ (ผู้ช่วยคาลิเบรตใช้)
          };
          dataPoints.push(newPt);
          updateRecordUI();
        }
      }catch(e){ console.error('poll: บันทึกจุดกราฟพลาด', e); }
      try{ drawChart(); }catch(e){ console.error('poll: วาดกราฟพลาด', e); }
      try{ if(userRole === 'admin'){ offRecord(data); drawOffsetChart(); offUpdateReadouts(); drawOffCalCurve(); offUpdateStatsTable(); } }catch(e){ console.error('poll: กราฟ Offset พลาด', e); }
      // v-pro: โหมดวัดมืออาชีพ — คำนวณ+วาดกราฟที่ 2 (ทำนาย) สดจากชุดข้อมูลเดียวกับกราฟที่ 1 ด้านบน ทุกครั้งที่โพลสำเร็จ
      try{
        var proBox = byId('proPredictBox');
        var proToggle = byId('proModeToggle');
        if(proToggle && proToggle.checked && dataPoints.length){
          if(proBox) proBox.style.display = 'block';
          var proPts = dataPoints.map(function(p){ return {t:p.t, aw:p.aw}; });
          var proPred = computeLivePrediction(proPts);
          drawPredictChart('proPredictChart', proPts, proPred, null, 'กราฟที่ 2: ทำนายค่าสมดุล (สด)');
          updatePredictReadouts('pro', proPred);
        } else if(proBox){
          proBox.style.display = 'none';
        }
      }catch(e){ console.error('poll: กราฟทำนายโหมดมืออาชีพพลาด', e); }
    }).catch(function(){
      failed = true;
      missedBeats++;
      if(missedBeats >= 1) setOnline(false, false);
    }).finally(function(){
      // v-fix: จุดเดียวที่ตั้งรอบถัดไป (เดิมมีทั้งใน .catch() และ .finally() พร้อมกัน ทำให้ยิงซ้อนกัน 2 ชุด
      // ทุกครั้งที่พลาดสักครั้ง แล้วยิ่งพลาดถี่ขึ้นเรื่อยๆ จนเว็บดูเหมือนต่อๆหลุดๆเป็นพักๆ) — ตอนนี้เหลือจุดเดียว
      // ในนี้เท่านั้นที่ตั้งเวลา ยิงถี่ขึ้น (400ms) เฉพาะตอนพลาด เพื่อกลับมาเชื่อมต่อไวๆ ปกติทุก 1 วิ
      pollInFlight = false;
      setTimeout(poll, failed ? 400 : 1000);
    });
  }

  function updateReadouts(data){
    var aw = (typeof data.aw === 'number') ? data.aw : 0;
    var rh = (typeof data.rh === 'number') ? data.rh : 0;
    var temp = (typeof data.temp === 'number') ? data.temp : -99;
    var t = (typeof data.t === 'number') ? data.t : 0;

    byId('gaugeNum').textContent = aw.toFixed(3);
    byId('gaugeNum').classList.toggle('danger', aw >= 0.9);
    var pct = Math.max(0, Math.min(1, aw));
    var CIRC = 283; // ความยาวเส้นโค้งของ gaugeArc โดยประมาณ (ตรงกับ stroke-dasharray ที่ตั้งไว้)
    byId('gaugeArc').style.strokeDashoffset = String(CIRC - pct * CIRC);

    byId('vRh').innerHTML = rh.toFixed(1) + '<span class="unit">%RH</span>';
    byId('fillRh').style.width = Math.max(0, Math.min(100, rh)) + '%';

    var tempTxt = (temp > -50) ? temp.toFixed(1) : '--.-';
    byId('vTemp').innerHTML = tempTxt + '<span class="unit">&deg;C</span>';
    if(temp > -50) byId('fillTemp').style.width = Math.max(0, Math.min(100, (temp/60)*100)) + '%';

    byId('awVal').textContent = aw.toFixed(3);
    byId('tempVal').innerHTML = tempTxt + '<span class="unit">&deg;C</span>';
    byId('timeVal').innerHTML = t.toFixed(1) + '<span class="unit">min</span>';
  }

  // ============================================================
  //  v-led-web: จุดสีสถานะ LED บนเว็บ — สีเดียวกับไฟ LED จริงบนตัวเครื่อง (อ่านจากฟิลด์ ledColor/ledBlink ใน /data)
  // ============================================================
  var LED_COLOR_HEX = {
    off:'#555', red:'#ff5d6c', green:'#33d17a', blue:'#4d7dff',
    yellow:'#ffd166', purple:'#c084fc', cyan:'#4dd0e1', white:'#f2f2f2'
  };
  var LED_COLOR_LABEL_TH = {
    off:'ปิด', red:'แดง', green:'เขียว', blue:'น้ำเงิน',
    yellow:'เหลือง', purple:'ม่วง (แจ้งเตือน)', cyan:'ฟ้าอมเขียว (พร้อมทำงาน)', white:'ขาว'
  };
  function updateLedStatus(data){
    var dot = byId('ledDot'), txt = byId('ledStatusText');
    if(!dot || !txt) return;
    var color = (typeof data.ledColor === 'string' && LED_COLOR_HEX[data.ledColor]) ? data.ledColor : 'off';
    var blinking = !!data.ledBlink;
    dot.style.background = LED_COLOR_HEX[color];
    dot.classList.toggle('blinking', blinking);
    txt.textContent = 'ไฟสถานะ: ' + (LED_COLOR_LABEL_TH[color] || color) + (blinking ? ' (กระพริบ)' : '');
  }

  // ============================================================
  //  v-dist: ระยะห่างโดยประมาณระหว่างตัวเครื่องกับอุปกรณ์ที่กำลังล็อกอิน/เชื่อมต่อดูหน้าเว็บนี้อยู่ (จากความแรงสัญญาณ Wi-Fi)
  // ============================================================
  function updateClientDistance(data){
    var el = byId('clientDistStat');
    if(!el) return;
    if(!data.clientConnected){
      el.textContent = 'ยังไม่พบอุปกรณ์เชื่อมต่อ';
      return;
    }
    var distM = (typeof data.clientDistM === 'number') ? data.clientDistM : NaN;
    var rssi = (typeof data.clientRssi === 'number') ? data.clientRssi : null;
    var distTxt = (!isNaN(distM)) ? ('~ ' + distM.toFixed(1) + ' ม.') : '–';
    el.textContent = distTxt + (rssi !== null ? ' (RSSI ' + rssi + ' dBm)' : '') + ' — ค่าประมาณคร่าว ๆ จากสัญญาณ Wi-Fi';
  }

  // ============================================================
  //  v-pid-tune: แผงจูน PID เทลเทียร์ (ขั้นตอนที่ 1) — เก็บ log อิสระจากกราฟ aw หลัก
  //  ทำงานเฉพาะแอดมิน ทุกจุดที่เก็บมี timestamp จริง (performance.now() + เวลานาฬิกาถ้าซิงก์แล้ว) เพื่อให้กราฟ/CSV
  //  ที่ส่งออกไปตอบกรรมการได้ว่าค่าตัวเลขแต่ละจุดมาจากการทดลองสดตอนไหน ไม่ได้เขียนมือ/พิมพ์เอาเอง
  // ============================================================
  var pidLog = [];                       // {ms, temp, target, pwmPct, kp, ki, kd}
  var PID_LOG_WINDOW_MS = 10 * 60 * 1000; // เก็บ 10 นาทีล่าสุดในกราฟ (ยาวพอเห็นหลายรอบแกว่ง ไม่หนักเกินไปให้เบราว์เซอร์วาด)
  var pidKpKiKdLoaded = false;           // โหลดค่าเกนปัจจุบันของบอร์ดมาใส่ในช่องกรอกครั้งแรกครั้งเดียว (ไม่ทับค่าที่ผู้ใช้กำลังพิมพ์อยู่)

  function updatePidTuner(data){
    if (typeof data.temp !== 'number' || typeof data.targetC !== 'number') return;

    if (!pidKpKiKdLoaded && typeof data.kp === 'number') {
      byId('pidKpIn').value = data.kp;
      byId('pidKiIn').value = data.ki;
      byId('pidKdIn').value = data.kd;
      pidKpKiKdLoaded = true;
    }

    var now = Date.now();
    pidLog.push({ ms: now, temp: data.temp, target: data.targetC, pwmPct: data.pwmPct || 0,
                  kp: data.kp, ki: data.ki, kd: data.kd });
    while (pidLog.length && (now - pidLog[0].ms) > PID_LOG_WINDOW_MS) pidLog.shift();

    var err = data.temp - data.targetC;
    byId('pidTempVal').innerHTML = data.temp.toFixed(2) + '<span class="unit">&deg;C</span>';
    byId('pidTargetVal').innerHTML = data.targetC.toFixed(1) + '<span class="unit">&deg;C</span>';
    byId('pidErrVal').innerHTML = (err >= 0 ? '+' : '') + err.toFixed(2) + '<span class="unit">&deg;C</span>';
    byId('pidPwmVal').innerHTML = (data.pwmPct || 0).toFixed(0) + '<span class="unit">%</span>';

    // แกว่ง (peak-to-peak) เฉพาะ 5 นาทีล่าสุดของ log นี้ — ตัวเลขเดียวที่บอกว่า "นิ่งพอหรือยัง" ตามที่ขอ
    var swingWindowMs = 5 * 60 * 1000;
    var mn = null, mx = null;
    for (var i = pidLog.length - 1; i >= 0; i--) {
      if (now - pidLog[i].ms > swingWindowMs) break;
      var t = pidLog[i].temp;
      if (mn === null || t < mn) mn = t;
      if (mx === null || t > mx) mx = t;
    }
    byId('pidSwingVal').innerHTML = (mn !== null) ? (mx - mn).toFixed(2) + '<span class="unit">&deg;C</span>' : '–';

    drawPidChart();
  }

  function drawPidChart(){
    var canvas = byId('pidChart');
    if (!canvas || !pidLog.length) return;
    var ctx = canvas.getContext('2d');
    var CW = canvas.width, CH = canvas.height;
    ctx.clearRect(0, 0, CW, CH);
    ctx.fillStyle = '#0e1822';
    ctx.fillRect(0, 0, CW, CH);

    var padL = 66, padR = 20, padT = 30, padB = 42;
    var plotW = CW - padL - padR, plotH = CH - padT - padB;
    var pwmBandH = plotH * 0.22;          // แถบล่างของกราฟใช้วาดแท่ง PWM %
    var tempPlotH = plotH - pwmBandH - 8;

    var tMin = pidLog[0].ms, tMax = pidLog[pidLog.length - 1].ms;
    if (tMax - tMin < 1000) tMax = tMin + 1000;

    var temps = pidLog.map(function(p){ return p.temp; }).concat(pidLog.map(function(p){ return p.target; }));
    var yMin = Math.min.apply(null, temps) - 0.3, yMax = Math.max.apply(null, temps) + 0.3;
    if (yMax - yMin < 0.6) { var mid = (yMax + yMin) / 2; yMin = mid - 0.3; yMax = mid + 0.3; }

    function xOf(ms){ return padL + (ms - tMin) / (tMax - tMin) * plotW; }
    function yOf(temp){ return padT + tempPlotH - (temp - yMin) / (yMax - yMin) * tempPlotH; }

    // ---- ชื่อกราฟ + ค่าเฉลี่ยอุณหภูมิ/PWM ของช่วงที่กำลังแสดงอยู่ (บอกชัดในกราฟเลย) ----
    var tSum = 0, pwmSum = 0;
    pidLog.forEach(function(p){ tSum += p.temp; pwmSum += (p.pwmPct || 0); });
    var avgTempPid = tSum / pidLog.length, avgPwmPid = pwmSum / pidLog.length;
    ctx.textAlign = 'left'; ctx.fillStyle = '#eaf2f8'; ctx.font = '700 14px sans-serif';
    ctx.fillText('กราฟจูน PID เทลเทียร์: อุณหภูมิจริง vs เป้าหมาย vs กำลัง PWM', padL, 16);
    ctx.textAlign = 'right'; ctx.fillStyle = '#8aa0b4'; ctx.font = '12px sans-serif';
    ctx.fillText('เฉลี่ยช่วงนี้ — อุณหภูมิ ' + avgTempPid.toFixed(2) + '°C, PWM ' + avgPwmPid.toFixed(0) + '%', CW - padR, 16);

    // เส้นกริด + แกน Y (อุณหภูมิ)
    ctx.strokeStyle = '#22303c'; ctx.fillStyle = '#8aa0b4'; ctx.font = '11px sans-serif'; ctx.lineWidth = 1;
    var gridLines = 4;
    for (var g = 0; g <= gridLines; g++) {
      var val = yMin + (yMax - yMin) * g / gridLines;
      var y = yOf(val);
      ctx.textAlign = 'left';
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
      ctx.fillText(val.toFixed(1) + '°C', 4, y + 4);
    }

    // เส้นเป้าหมาย (ประขาว)
    ctx.strokeStyle = '#ffffff'; ctx.setLineDash([6, 4]); ctx.lineWidth = 1.5;
    ctx.beginPath();
    pidLog.forEach(function(p, i){ var x = xOf(p.ms), y = yOf(p.target); if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); });
    ctx.stroke(); ctx.setLineDash([]);

    // เส้นอุณหภูมิจริง (ฟ้า)
    ctx.strokeStyle = '#07d9ff'; ctx.lineWidth = 2;
    ctx.beginPath();
    pidLog.forEach(function(p, i){ var x = xOf(p.ms), y = yOf(p.temp); if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); });
    ctx.stroke();

    // แท่ง PWM % ด้านล่าง (ส้ม)
    var pwmBaseY = padT + tempPlotH + 8 + pwmBandH;
    ctx.textAlign = 'left'; ctx.fillStyle = '#8aa0b4'; ctx.fillText('กำลัง PWM เทลเทียร์ (%)', padL, pwmBaseY - pwmBandH - 4);
    ctx.fillStyle = 'rgba(255,153,51,0.65)';
    var barW = Math.max(1, plotW / pidLog.length);
    pidLog.forEach(function(p){
      var x = xOf(p.ms);
      var h = (p.pwmPct / 100) * pwmBandH;
      ctx.fillRect(x, pwmBaseY - h, barW, h);
    });
    ctx.strokeStyle = '#22303c';
    ctx.beginPath(); ctx.moveTo(padL, pwmBaseY); ctx.lineTo(padL + plotW, pwmBaseY); ctx.stroke();

    // ---- ชื่อแกน X/Y ----
    ctx.textAlign = 'center'; ctx.fillStyle = '#c3d1de'; ctx.font = '600 11px sans-serif';
    ctx.fillText('เวลา (ล่าสุด ' + Math.round((tMax - tMin) / 1000) + ' วินาทีที่ผ่านมา)', padL + plotW / 2, CH - 6);
    ctx.save(); ctx.translate(12, padT + tempPlotH / 2); ctx.rotate(-Math.PI / 2);
    ctx.textAlign = 'center'; ctx.fillStyle = '#07d9ff'; ctx.font = '600 11px sans-serif';
    ctx.fillText('อุณหภูมิ (°C)', 0, 0); ctx.restore();
  }

  function pidCsvHeader(){
    return 'timestamp_iso,elapsed_s,temp_C,target_C,error_C,pwm_pct,kp,ki,kd\n';
  }
  function pidLogToCsv(){
    if (!pidLog.length) return pidCsvHeader();
    var t0 = pidLog[0].ms;
    var rows = pidLog.map(function(p){
      return new Date(p.ms).toISOString() + ',' + ((p.ms - t0) / 1000).toFixed(1) + ',' +
        p.temp.toFixed(2) + ',' + p.target.toFixed(1) + ',' + (p.temp - p.target).toFixed(2) + ',' +
        (p.pwmPct || 0).toFixed(1) + ',' + p.kp + ',' + p.ki + ',' + p.kd;
    });
    return pidCsvHeader() + rows.join('\n') + '\n';
  }

  function initPidTunerControls(){
    byId('pidApplyBtn').addEventListener('click', function(){
      var kp = byId('pidKpIn').value, ki = byId('pidKiIn').value, kd = byId('pidKdIn').value;
      var msg = byId('pidApplyMsg');
      msg.textContent = 'กำลังส่ง...';
      fetch('/pidset?kp=' + encodeURIComponent(kp) + '&ki=' + encodeURIComponent(ki) + '&kd=' + encodeURIComponent(kd))
        .then(function(r){ if (!r.ok) throw new Error('bad'); return r.text(); })
        .then(function(){ msg.textContent = '✓ ใช้ค่าแล้วเมื่อ ' + new Date().toLocaleTimeString(); })
        .catch(function(){ msg.textContent = '✗ ส่งค่าไม่สำเร็จ (เช็คช่วงค่า Kp 0-200 / Ki 0-20 / Kd 0-100)'; });
    });
    byId('pidResetBtn').addEventListener('click', function(){ pidLog = []; drawPidChart(); });
    byId('offResetBtn').addEventListener('click', function(){ offLog = []; drawOffsetChart(); drawOffCalCurve(); offUpdateStatsTable(); });
    byId('offCsvBtn').addEventListener('click', offCsv);
    byId('offPngBtn').addEventListener('click', downloadOffsetFormalPng);
    byId('offCalPngBtn').addEventListener('click', downloadOffCalFormalPng);
    byId('offStatsPngBtn').addEventListener('click', downloadOffStatsFormalPng);
    if(byId('offCalZoneSel')) byId('offCalZoneSel').addEventListener('change', function(){ drawOffCalCurve(); });
    if(byId('offRefInput')) byId('offRefInput').addEventListener('input', function(){ drawOffsetChart(); offUpdateStatsTable(); });
    byId('pidCsvBtn').addEventListener('click', function(){
      downloadBlob(pidLogToCsv(), 'text/csv', 'pid_tuning_log_' + Date.now() + '.csv');
    });
    byId('pidAutoTuneBtn').addEventListener('click', function(){
      byId('pidAutoTuneMsg').textContent = 'กำลังเริ่ม...';
      fetch('/pidautotune?start=1')
        .then(function(r){ if (!r.ok) return r.text().then(function(t){ throw new Error(t); }); return r.text(); })
        .then(function(){ pidAutoTunePollStart(); })
        .catch(function(e){ byId('pidAutoTuneMsg').textContent = '✗ เริ่มไม่ได้: ' + e.message; });
    });
    byId('pidAutoTuneCancelBtn').addEventListener('click', function(){
      fetch('/pidautotune?cancel=1').then(function(){ pidAutoTunePollStop('ยกเลิกแล้ว — กลับไปใช้ค่าก่อนหน้า'); });
    });
  }

  // v20: โพลสถานะ Auto-Tune ทุก 1.5 วิระหว่างกำลังจูน จนกว่าจะจบ (สำเร็จ/ล้มเหลว) แล้วหยุดโพลเอง
  var pidAutoTunePollTimer = null;
  function pidAutoTunePollStart(){
    byId('pidAutoTuneBtn').style.display = 'none';
    byId('pidAutoTuneCancelBtn').style.display = '';
    if (pidAutoTunePollTimer) clearInterval(pidAutoTunePollTimer);
    pidAutoTunePollTimer = setInterval(pidAutoTunePoll, 1500);
    pidAutoTunePoll();
  }
  function pidAutoTunePollStop(finalMsg){
    if (pidAutoTunePollTimer) { clearInterval(pidAutoTunePollTimer); pidAutoTunePollTimer = null; }
    byId('pidAutoTuneBtn').style.display = '';
    byId('pidAutoTuneCancelBtn').style.display = 'none';
    if (finalMsg) byId('pidAutoTuneMsg').textContent = finalMsg;
    pidKpKiKdLoaded = false; // บังคับดึง Kp/Ki/Kd ล่าสุด (ที่ auto-tune อาจเปลี่ยนไปแล้ว) มาใส่ในช่องกรอกใหม่รอบถัดไป
  }
  function pidAutoTunePoll(){
    fetch('/pidautotune/status').then(function(r){ return r.json(); }).then(function(d){
      // state: 0=idle 1=running 2=done 3=failed
      if (d.state === 1) {
        byId('pidAutoTuneMsg').textContent = 'กำลังจูน... ครบ ' + d.stepsDone + '/' + d.stepsNeeded + ' ช่วงแกว่ง (' + d.elapsedS + ' วิ, ห้อง~' + d.envC.toFixed(0) + '°C)';
      } else if (d.state === 2) {
        pidAutoTunePollStop('✓ จูนสำเร็จ: Ku=' + d.ku.toFixed(2) + ' Pu=' + d.pu.toFixed(1) + 's -> Kp=' + d.kp.toFixed(2) + ' Ki=' + d.ki.toFixed(3) + ' Kd=' + d.kd.toFixed(2) + ' (บันทึกไว้สำหรับห้อง~' + d.envC.toFixed(0) + '°C แล้ว)');
      } else if (d.state === 3) {
        pidAutoTunePollStop('✗ จูนไม่สำเร็จ: ' + d.reason);
      } else {
        pidAutoTunePollStop(null);
      }
    }).catch(function(){ /* เว็บอาจหลุดชั่วคราว - โพลรอบถัดไปจะลองใหม่เอง */ });
  }

  // v-fix: เช็คว่าค่า aw ที่อ่านสดจากบอร์ดตอนนี้ "นิ่ง" หรือยัง โดยดูช่วงกว้างสุด-ต่ำสุดในหน้าต่างเวลาล่าสุด
  // (STABLE_POPUP_WINDOW_MS) ถ้าแคบพอ (<= STABLE_POPUP_TOL) ถือว่านิ่งแล้ว เด้งป็อปอัปแจ้งครั้งเดียว แล้วรอ
  // จนกว่าค่าจะแกว่งออกไปเกิน STABLE_POPUP_EXIT_TOL ก่อนถึงจะเด้งซ้ำได้อีกรอบ (กันเด้งถี่ๆ ตอนแกว่งอยู่พอดีขอบ)
  // ทำงานทุกครั้งที่หน้าเว็บอ่านค่าได้สำเร็จ ไม่ว่าจะกด "เริ่มบันทึกกราฟ" ไว้หรือไม่ก็ตาม
  function updateStabilityPopup(data){
    if(isHeatPhase(data) || data.heater){ stabBufWeb.length = 0; stablePopupShown = false; return; } // v12: ช่วงฮีต/รอเย็นไม่นับความนิ่ง
    var awOk = (typeof data.aw === 'number') && !isNaN(data.aw) && !data.faultSHT;
    if(!awOk) return; // อ่านค่าไม่ได้ชั่วขณะ ข้ามรอบนี้ไปก่อน ไม่เอามาปนกับหน้าต่างความนิ่ง

    var now = performance.now();
    stabBufWeb.push({ tMs: now, aw: data.aw, pwmOk: (data.pwmSteady === true) });
    while(stabBufWeb.length && (now - stabBufWeb[0].tMs) > STABLE_POPUP_WINDOW_MS) stabBufWeb.shift();

    // ต้องมีข้อมูลครอบคลุมเกือบเต็มหน้าต่างเวลาก่อน ถึงจะตัดสินว่านิ่งได้ (กันตัดสินเร็วเกินไปตอนเพิ่งเริ่มวัด)
    if(!stabBufWeb.length || (now - stabBufWeb[0].tMs) < STABLE_POPUP_WINDOW_MS * 0.75) return;

    var mn = stabBufWeb[0].aw, mx = stabBufWeb[0].aw;
    stabBufWeb.forEach(function(p){ if(p.aw < mn) mn = p.aw; if(p.aw > mx) mx = p.aw; });
    var range = mx - mn;

    // v18: "คงที่" อิง PWM ด้วย — ถ้า PWM เทลเทียร์นิ่งตลอดหน้าต่างนี้ (ลูปควบคุมเข้าสมดุล) ค่าที่ขึ้น-ลงหน่อยเป็นแค่ ripple ของอุณหภูมิ ไม่ใช่ยังไม่นิ่ง
    var pwmSteadyAll = stabBufWeb.every(function(p){ return p.pwmOk; });

    // v21: PWM นิ่งตลอดหน้าต่าง (ตรวจเจอทุกจุด ไม่ใช่แค่บางครั้ง) -> นับว่า "นิ่ง" เสมอ ไม่จำกัดช่วงกว้างอีกต่อไป
    // (เหมือนเฟิร์มแวร์ v21 ดู stabFlat()) แจ้งค่าเป็นจุดกึ่งกลางการแกว่ง (สูงสุด+ต่ำสุด)/2 แทน data.aw สด ๆ ตัวเดียว
    // ให้ตรงกับค่าที่เฟิร์มแวร์จะล็อกจริง (ดู updateStabilityWindow() ฝั่งเฟิร์มแวร์)
    if(pwmSteadyAll){
      if(!stablePopupShown){ stablePopupShown = true; showStableToast((mn + mx) / 2); }
      return;
    }

    if(range <= STABLE_POPUP_TOL){
      if(!stablePopupShown){ stablePopupShown = true; showStableToast(data.aw); }
    } else if(range > STABLE_POPUP_EXIT_TOL){
      stablePopupShown = false; // แกว่งออกไปพอสมควรแล้ว อนุญาตให้เด้งแจ้งอีกครั้งเมื่อนิ่งใหม่
    }
  }

  var stableToastTimer = null;
  function showStableToast(awVal){
    var el = byId('stableToast');
    if(!el) return;
    el.textContent = '✓ ค่าคงที่แล้ว — aw ≈ ' + awVal.toFixed(3);
    el.classList.add('show');
    if(stableToastTimer) clearTimeout(stableToastTimer);
    stableToastTimer = setTimeout(function(){ el.classList.remove('show'); }, 4000);
  }

  // v12: เฟสที่ห้ามเก็บค่าลงกราฟ (เซนเซอร์กำลังฮีตหรือกำลังเย็นลง) — ค่า phase มาจากเฟิร์มแวร์ใน /data
  function isHeatPhase(data){
    var p = data && data.phase;
    return p === 'preheat' || p === 'precool' || p === 'postheat' || p === 'postcool';
  }
  var PHASE_TEXT = {
    preheat:  '🔥 กำลังฮีตเตอร์เซนเซอร์ SHT ก่อนวัด — ค่าช่วงนี้ไม่ถูกบันทึกลงกราฟ/CSV',
    precool:  '⏳ รอเซนเซอร์ SHT เย็นลงหลังฮีต — เครื่องจะเริ่มวัดจริงเมื่อครบเวลา (ค่าช่วงนี้ไม่ถูกบันทึกลงกราฟ/CSV)',
    postheat: '🔥 กำลังฮีตเตอร์เซนเซอร์ SHT หลังวัด (ไล่ไอน้ำตกค้าง) — ค่าช่วงนี้ไม่ถูกบันทึกลงกราฟ/CSV',
    postcool: '⏳ รอเซนเซอร์ SHT เย็นลงหลังฮีต — ค่าช่วงนี้ไม่ถูกบันทึกลงกราฟ/CSV'
  };
  function updatePhaseBanner(data){
    var pb = byId('phaseBanner'), qb = byId('promptBanner');
    if(pb){ var txt = isHeatPhase(data) ? PHASE_TEXT[data.phase] : ''; pb.textContent = txt; pb.classList.toggle('show', !!txt); }
    if(qb){
      var on = !!data.savePrompt;
      qb.textContent = on ? 'ℹ️ ค่าบนจอเครื่องนิ่งแล้ว — เครื่องกำลังรอให้เลือก "บันทึก / ไม่บันทึก" ที่หน้าจอตัวเครื่อง (การวัดยังดำเนินต่อไป)' : '';
      qb.classList.toggle('show', on);
    }
  }

  function updateFaultBanner(data){
    var msgs = [];
    if(data.faultSHT) msgs.push('เซนเซอร์ความชื้น (' + (data.humiditySensorType || 'SHT/DHT') + ') อ่านค่าไม่ได้ — ตรวจสายต่อ/ตรวจว่าเสียบเซนเซอร์ไว้หรือยัง');
    if(data.faultTemp) msgs.push('เซนเซอร์อุณหภูมิ (DS18B20) อ่านค่าไม่ได้ — ตรวจสายต่อ');
    if(data.roomTooHot) msgs.push('ห้องร้อนเกินกำลังเทลเทียร์ — อุณหภูมิยังไม่นิ่งที่เป้าหมาย');
    if(data.calDue) msgs.push('คาลิเบรตเกินอายุที่แนะนำ (' + data.calAgeDays + ' วัน) — ควรคาลิเบรตซ้ำ');
    if(data.noiseWarning) msgs.push('ค่าล่าสุดกระเพื่อมกว้างผิดปกติระหว่างช่วงนิ่ง — ควรตรวจเซนเซอร์');
    if(data.coldRoom) msgs.push('ห้องเย็นกว่าเป้าหมาย — เทลเทียร์ของเครื่องทำความเย็นได้อย่างเดียว อุณหภูมิห้องวัดจึงลอยตามแอร์ (ค่า aw ของตัวอย่างเปลี่ยนตามอุณหภูมิ) ควรตั้งแอร์ให้ไม่เย็นกว่า 25 °C หรือทิ้งเครื่องให้อุณหภูมิเข้าที่ก่อนวัด');
    if(typeof data.dT === 'number' && data.shtT > -50 && Math.abs(data.dT) > 1.0)
      msgs.push('ตัวเซนเซอร์ความชื้นกับตัวอย่างอุณหภูมิต่างกัน ' + data.dT.toFixed(1) + ' °C — ' +
                ((valueMode !== 'RAW') ? 'เครื่องชดเชยค่า %RH ให้แล้ว แต่ควรแก้ที่ต้นเหตุ (ลมแอร์เป่าใส่ตัวเครื่อง/ความร้อนจากบอร์ดใกล้เซนเซอร์)' : 'ค่า %RH จะคลาดประมาณ 6% ต่อ 1 °C (โหมด RAW ไม่ชดเชย)'));
    // v15: ตัวตรวจแนวโน้มไหลช้า (slow-trend gate) กำลังยับยั้งการล็อก "นิ่ง" — มักเกิดตอนวัดตัวอย่างที่ %RH
    // ต่างจากตัวอย่างก่อนหน้ามาก (เช่น น้ำต่อด้วยเกลืออิ่มตัว) เซนเซอร์ยังคายความชื้นตกค้างช้าๆ อยู่ ไม่ใช่ข้อผิดพลาด
    // แค่ต้องรอนานกว่าปกติกว่าจะถึงจุดสมดุลจริง — ข้อความนี้ไม่ใช่คำเตือน (ไม่แดง) จึงไม่ต้องเติม '⚠️' ซ้ำในนี้
    if(data.stillDrifting) msgs.push('ℹ️ กำลังวัดต่อ: ค่ายังไหลช้าๆ อยู่จริง (ยังไม่ใช่จุดสมดุล) มักเกิดถ้าตัวอย่างก่อนหน้ามี %RH ต่างจากตัวอย่างนี้มาก — รอต่อได้ ระบบจะยังไม่ล็อกค่าจนกว่าจะนิ่งจริง');
    var el = byId('faultBanner');
    if(msgs.length){ el.textContent = '⚠️ ' + msgs.join(' · '); el.classList.add('show'); }
    else { el.classList.remove('show'); }
  }

  function updateAuditPanel(data){
    byId('clockStat').textContent = data.clockVerified ? 'ซิงก์แล้ว' : (data.clockSynced ? 'ค่าค้าง (ยังไม่ยืนยันรอบนี้)' : 'ยังไม่ตั้ง');
    byId('calAgeStat').textContent = (data.calAgeDays < 0) ? 'ไม่ทราบ' : (data.calAgeDays + ' วันก่อน');
    byId('noiseStat').textContent = data.noiseWarning ? 'ควรตรวจสอบ' : 'ปกติ';
    byId('opStat').textContent = data.operator ? data.operator : '(ยังไม่ตั้ง)';
    // v14: ส่วนต่างอุณหภูมิระหว่างตัวชิป SHT กับตัวอย่าง (DS18B20) และตัวคูณที่ใช้ชดเชย %RH (โหมดคาลิเบรต)
    var dtEl = byId('dtStat'), gfEl = byId('gfStat');
    if(dtEl){
      var hasDt = (typeof data.shtT === 'number' && data.shtT > -50 && typeof data.dT === 'number');
      dtEl.textContent = hasDt ? ((data.dT >= 0 ? '+' : '') + data.dT.toFixed(2) + ' °C (ชิป ' + data.shtT.toFixed(1) + ' °C)') : '–';
      dtEl.style.color = (hasDt && Math.abs(data.dT) > 1.0) ? '#ffb4b4' : '';
    }
    if(gfEl) gfEl.textContent = (valueMode === 'RAW') ? 'ไม่ใช้ (โหมด RAW)' : ((typeof data.gf === 'number') ? ('×' + data.gf.toFixed(4)) : '–');
    // v-slope-live: ตัวคูณชดเชยอุณหภูมิเปลี่ยนได้ตลอดตามอุณหภูมิห้อง/ตัวอย่าง ณ ขณะนั้น จึงรีเฟรชบรรทัดนี้ในช่อง
    // "จุดที่ยืด/หดสเกล" ทุกรอบโพลด้วย ให้เห็นภาพรวมความแม่นยำครบทั้งจูนปกติ (ตาราง) และจูนตามอุณหภูมิ (ตัวคูณ) พร้อมกัน
    try{ renderCalSlopeBox(); }catch(e){}
  }

  // ============================================================
  //  ควบคุมการบันทึกกราฟ (ฝั่งเบราว์เซอร์ล้วนๆ ไม่สั่งบอร์ด)
  // ============================================================
  function updateRecordUI(){
    var btn = byId('recToggleBtn'), dot = byId('recDot'), stat = byId('recStatus');
    if(!btn || !dot || !stat) return;
    dot.classList.toggle('on', recording);
    if(recording){
      btn.textContent = 'หยุดบันทึก';
      btn.classList.remove('primary'); btn.classList.add('stop');
      stat.innerHTML = 'กำลังบันทึก… (<span class="rec-count">' + dataPoints.length + '</span> จุดแล้ว) กด "หยุดบันทึก" เมื่อพอแล้ว';
    } else {
      btn.textContent = 'เริ่มบันทึกกราฟ';
      btn.classList.remove('stop'); btn.classList.add('primary');
      stat.innerHTML = dataPoints.length
        ? ('หยุดบันทึกแล้ว (<span class="rec-count">' + dataPoints.length + '</span> จุด) — ดาวน์โหลด CSV ได้เลย หรือกดเริ่มใหม่เพื่อบันทึกต่อ')
        : 'ยังไม่เริ่มบันทึก — ตัวเลขด้านบนจะอัปเดตสดตลอดเวลาอยู่แล้ว กดเริ่มบันทึกเพื่อเก็บข้อมูลเข้ากราฟ/CSV ได้ยาวไม่จำกัดเวลา จนกว่าจะพอ';
    }
  }

  // ============================================================
  //  วาดกราฟด้วย Canvas ล้วน
  // ============================================================
  var canvas = byId('chart');
  var cctx = canvas.getContext('2d');
  var CW = canvas.width, CH = canvas.height;
  var M = { left:74, right:74, top:34, bottom:74 };

  function getVisibleSeries(){
    var series = [];
    if(byId('liveToggle').checked){
      series.push({ name:'ปัจจุบัน (Live)', color:'#eaf2f8', points:dataPoints });
    }
    savedRuns.filter(function(r){ return r.visible; }).forEach(function(r){
      series.push({ name:r.name, color:r.color, points:r.points });
    });
    // v-fix: เดิมแกนเวลา (t) ของแต่ละรอบคือเวลาสะสมตั้งแต่บอร์ดบูต/เปิดแดชบอร์ด ทำให้เวลาเอากราฟของ
    // สองรอบที่บันทึกคนละช่วงเวลามาซ้อนกัน กราฟจะ "ต่อกัน"/เหลื่อมกันไปคนละตำแหน่งบนแกน X แทนที่จะเริ่ม
    // จากศูนย์พร้อมกัน จึงปรับให้ตอนวาดกราฟ ทุกเส้นเริ่มนับ t=0 จากจุดแรกของตัวเอง (เฉพาะตอนวาด ไม่แก้ไข
    // ข้อมูลดิบที่เก็บไว้ ดังนั้น CSV export ยังคงมีเวลาจริงตามที่วัดได้เหมือนเดิม)
    series = series.map(function(s){
      if(!s.points.length) return s;
      var t0 = s.points[0].t;
      return {
        name: s.name, color: s.color,
        points: s.points.map(function(p){ return { t: p.t - t0, aw:p.aw, raw:p.raw, temp:p.temp, roomAw:p.roomAw, roomRaw:p.roomRaw }; })
      };
    });
    return series;
  }

  function seriesAverages(pts){
    var awSum = 0, awN = 0, tempSum = 0, tempN = 0;
    pts.forEach(function(p){
      if(typeof p.aw === 'number' && !isNaN(p.aw)){ awSum += p.aw; awN++; }
      if(typeof p.temp === 'number' && !isNaN(p.temp) && p.temp > -50){ tempSum += p.temp; tempN++; }
    });
    return { avgAw: awN ? awSum/awN : null, avgTemp: tempN ? tempSum/tempN : null };
  }

  function drawChart(){
    cctx.clearRect(0,0,CW,CH);
    cctx.fillStyle = '#0e1822';
    cctx.fillRect(0,0,CW,CH);

    var plotW = CW - M.left - M.right;
    var plotH = CH - M.top - M.bottom;
    var series = getVisibleSeries();

    var maxT = 5;
    series.forEach(function(s){
      if(s.points.length) maxT = Math.max(maxT, Math.ceil(s.points[s.points.length-1].t));
    });
    var tempMin = 0, tempMax = 60;

    // ซูมแกน Y ซ้ายให้พอดีกับช่วงข้อมูลที่กำลังวาด (aw และ raw ถ้าเปิดซ้อน) ปิดได้ด้วย checkbox
    var yLoL = 0, yHiL = 1;
    var yzEl = byId('yZoomToggle');
    if(yzEl && yzEl.checked){
      var rawOnZ = (userRole === 'admin' && byId('rawToggle') && byId('rawToggle').checked);
      var zlo = Infinity, zhi = -Infinity;
      series.forEach(function(s){ s.points.forEach(function(p){
        if(typeof p.aw === 'number' && !isNaN(p.aw)){ if(p.aw < zlo) zlo = p.aw; if(p.aw > zhi) zhi = p.aw; }
        if(rawOnZ && typeof p.raw === 'number' && !isNaN(p.raw)){ if(p.raw < zlo) zlo = p.raw; if(p.raw > zhi) zhi = p.raw; }
        if(!rawOnZ && typeof p.roomAw === 'number' && !isNaN(p.roomAw)){ if(p.roomAw < zlo) zlo = p.roomAw; if(p.roomAw > zhi) zhi = p.roomAw; }
      }); });
      if(isFinite(zlo) && isFinite(zhi)){
        var zmid = (zlo + zhi) / 2, zhalf = Math.max(zhi - zlo, 0.02) / 2 * 1.3;
        yLoL = zmid - zhalf; yHiL = zmid + zhalf;
        if(yLoL < 0){ yHiL -= yLoL; yLoL = 0; }
        if(yHiL > 1){ yLoL -= (yHiL - 1); yHiL = 1; }
        if(yLoL < 0) yLoL = 0;
      }
    }
    var yDecL = (yHiL - yLoL) < 0.05 ? 3 : ((yHiL - yLoL) < 0.5 ? 2 : 1);

    function xPix(t){ return M.left + (t/maxT)*plotW; }
    function yPixAw(v){ return M.top + plotH - ((Math.max(yLoL,Math.min(yHiL,v))-yLoL)/(yHiL-yLoL))*plotH; }
    function yPixTemp(v){ return M.top + plotH - ((v-tempMin)/(tempMax-tempMin))*plotH; }

    // ---- ชื่อกราฟ (บอกชัดว่ากราฟนี้คือกราฟอะไร) ----
    cctx.textAlign = 'left'; cctx.fillStyle = '#eaf2f8'; cctx.font = '700 17px sans-serif';
    cctx.fillText('กราฟแนวโน้ม Water Activity (aw) และอุณหภูมิ ตามเวลา', M.left, 22);

    cctx.strokeStyle = '#28394a';
    cctx.lineWidth = 1.5;
    cctx.strokeRect(M.left, M.top, plotW, plotH);

    // เส้นกริด + สเกล
    cctx.font = '13px sans-serif';
    for(var i=0;i<=5;i++){
      var v = yLoL + (yHiL-yLoL)*(i/5), y = yPixAw(v);
      cctx.strokeStyle = 'rgba(255,255,255,0.05)';
      cctx.beginPath(); cctx.moveTo(M.left,y); cctx.lineTo(M.left+plotW,y); cctx.stroke();
      cctx.textAlign = 'right'; cctx.fillStyle = '#38d9c9';
      cctx.fillText(v.toFixed(yDecL), M.left-8, y+4);
      var tv = tempMin + (tempMax-tempMin)*(i/5);
      cctx.textAlign = 'left'; cctx.fillStyle = '#ff9f5a';
      cctx.fillText(Math.round(tv), M.left+plotW+8, y+4);
    }
    var xTicks = 6;
    cctx.fillStyle = '#8fa3b5';
    for(var j=0;j<=xTicks;j++){
      var t = (maxT/xTicks)*j, x = xPix(t);
      cctx.strokeStyle = 'rgba(255,255,255,0.05)';
      cctx.beginPath(); cctx.moveTo(x,M.top); cctx.lineTo(x,M.top+plotH); cctx.stroke();
      cctx.textAlign = 'center'; cctx.fillText(t.toFixed(0)+'m', x, M.top+plotH+20);
    }

    // ---- ชื่อแกน (X / Y ซ้าย / Y ขวา) ----
    cctx.textAlign = 'center'; cctx.font = '600 13px sans-serif'; cctx.fillStyle = '#c3d1de';
    cctx.fillText('เวลา (นาที)', M.left + plotW/2, M.top + plotH + 44);
    cctx.save(); cctx.translate(16, M.top + plotH/2); cctx.rotate(-Math.PI/2);
    cctx.textAlign = 'center'; cctx.fillStyle = '#38d9c9'; cctx.font = '600 13px sans-serif';
    cctx.fillText('Water Activity (aw)', 0, 0); cctx.restore();
    cctx.save(); cctx.translate(CW-16, M.top + plotH/2); cctx.rotate(-Math.PI/2);
    cctx.textAlign = 'center'; cctx.fillStyle = '#ff9f5a'; cctx.font = '600 13px sans-serif';
    cctx.fillText('อุณหภูมิ (°C)', 0, 0); cctx.restore();

    // v-room-aw: เส้นประแนวนอน = AW ห้องที่จับได้ก่อนเริ่มรอบวัด
    // วาดก่อนเส้นตัวอย่าง เพื่อให้เห็นชัดว่าค่าปัจจุบันเปลี่ยนจาก baseline ห้องเท่าไร
    series.forEach(function(s){
      var roomPt = s.points.find(function(p){ return typeof p.roomAw === 'number' && !isNaN(p.roomAw); });
      if(!roomPt) return;
      cctx.setLineDash([10,6]); cctx.strokeStyle = '#c084fc'; cctx.globalAlpha = 0.9; cctx.lineWidth = 1.8;
      cctx.beginPath(); cctx.moveTo(M.left, yPixAw(roomPt.roomAw)); cctx.lineTo(M.left + plotW, yPixAw(roomPt.roomAw)); cctx.stroke();
      cctx.setLineDash([]); cctx.globalAlpha = 1;
      cctx.textAlign = 'left'; cctx.font = '12px sans-serif'; cctx.fillStyle = '#c084fc';
      cctx.fillText('AW ห้อง ' + roomPt.roomAw.toFixed(3), M.left + 8, yPixAw(roomPt.roomAw) - 6);
    });

    // เส้นข้อมูล
    series.forEach(function(s){
      if(s.points.length < 2) return;
      cctx.strokeStyle = s.color; cctx.lineWidth = 2.2; cctx.beginPath();
      s.points.forEach(function(p,idx){
        var x = xPix(p.t), y = yPixAw(p.aw);
        if(idx===0) cctx.moveTo(x,y); else cctx.lineTo(x,y);
      });
      cctx.stroke();

      // v-web-ctrl: แอดมินเท่านั้น — ซ้อนเส้นกราฟ "ค่าจริง" (RAW ก่อนคาลิเบรต) ทับเส้น aw คาลิเบรตแล้ว เพื่อเทียบกันสด ๆ
      if(userRole === 'admin' && byId('rawToggle') && byId('rawToggle').checked){
        cctx.setLineDash([2,3]);
        cctx.strokeStyle = '#ffd166'; cctx.globalAlpha = 0.85; cctx.lineWidth = 1.8; cctx.beginPath();
        s.points.forEach(function(p,idx){
          if(typeof p.raw !== 'number' || isNaN(p.raw)) return;
          var x = xPix(p.t), y = yPixAw(p.raw);
          if(idx===0) cctx.moveTo(x,y); else cctx.lineTo(x,y);
        });
        cctx.stroke();
        cctx.setLineDash([]); cctx.globalAlpha = 1;
      }

      cctx.setLineDash([5,4]);
      cctx.strokeStyle = s.color; cctx.globalAlpha = 0.55; cctx.lineWidth = 1.6; cctx.beginPath();
      s.points.forEach(function(p,idx){
        if(p.temp <= -50) return;
        var x = xPix(p.t), y = yPixTemp(p.temp);
        if(idx===0) cctx.moveTo(x,y); else cctx.lineTo(x,y);
      });
      cctx.stroke();
      cctx.setLineDash([]); cctx.globalAlpha = 1;
    });

    // ---- ป้ายชื่อเส้น + ค่าเฉลี่ย aw/อุณหภูมิ ของแต่ละเส้นที่กำลังแสดง (มุมขวาบน) ----
    var legendItems = series.filter(function(s){ return s.points.length; });
    if(legendItems.length){
      var lw = 300, lh = 18 * legendItems.length + 10;
      var lx = M.left + plotW - lw - 6, ly = M.top + 6;
      cctx.fillStyle = 'rgba(6,12,18,0.78)'; cctx.fillRect(lx, ly, lw, lh);
      cctx.strokeStyle = '#28394a'; cctx.lineWidth = 1; cctx.strokeRect(lx, ly, lw, lh);
      legendItems.forEach(function(s, idx){
        var avgs = seriesAverages(s.points);
        var ry = ly + 16 + idx * 18;
        cctx.fillStyle = s.color; cctx.fillRect(lx + 8, ry - 9, 10, 10);
        cctx.textAlign = 'left'; cctx.font = '12px sans-serif'; cctx.fillStyle = '#dce6ef';
        var avgAwTxt = (avgs.avgAw !== null) ? avgs.avgAw.toFixed(3) : '–';
        var avgTempTxt = (avgs.avgTemp !== null) ? avgs.avgTemp.toFixed(1)+'°C' : '–';
        cctx.fillText(s.name + ' — aw เฉลี่ย ' + avgAwTxt + ', อุณหภูมิเฉลี่ย ' + avgTempTxt, lx + 24, ry);
      });
    }

    if(!series.some(function(s){ return s.points.length; })){
      cctx.fillStyle = '#8fa3b5'; cctx.textAlign = 'center'; cctx.font = '16px sans-serif';
      cctx.fillText('ยังไม่มีข้อมูล — กด "เริ่มบันทึกกราฟ" ด้านบน', M.left+plotW/2, M.top+plotH/2);
    }
  }

  // ============================================================
  //  v-trend-offset: กราฟ Offset (แอดมิน) — เก็บสดทุกโพลตอนเครื่องวัดจริง ไม่ต้องกด "เริ่มบันทึกกราฟ"
  //  ฟิลด์จากบอร์ด (/data): toBase = aw ตารางก่อนปรับ, toOff = offset ที่ใช้, toTgt = aw เป้าหมายของโซน,
  //  toZ = โซนต่อเนื่อง 0..2, toW = ตัวถ่วง 0..1, toSl = ความชัน raw (ต่อนาที)
  // ============================================================
  var offLog = [];
  var OFF_LOG_MAX = 3600;
  function offRecord(data){
    if(typeof data.toBase !== 'number' || isNaN(data.toBase)) return;
    if(isHeatPhase(data) || data.heater) return;
    offLog.push({ t:data.t, raw:data.raw, base:data.toBase, out:data.aw, off:data.toOff, tgt:data.toTgt,
                  z:data.toZ, w:data.toW, sl:data.toSl,
                  eqRaw: (typeof data.toEqRaw === 'number' && data.toEqRaw >= 0) ? data.toEqRaw : null,
                  eqUsed: !!data.toEqUsed });
    if(offLog.length > OFF_LOG_MAX) offLog.shift();
  }
  function offZoneName(z){
    if(z < 0.5) return 'โซน 0: CaCl2/MgCl2 (ปรับไม่เกิน ±0.10) — เช่น ขนมกรอบ/มาม่า';
    if(z < 1.5) return 'โซน 1: MgCl2→NaCl (เพดาน 0.75)';
    if(z < 2.5) return 'โซน 2: NaCl→KCl (เพดาน 0.90)';
    return 'โซน 3: KCl→น้ำบริสุทธิ์ (เพดาน 0.99) — เช่น น้ำ/วุ้นเส้น';
  }
  function offFormulaText(p){
    var s = 'aw = base + w×(target − base) = ' + p.base.toFixed(3) + ' + ' + p.w.toFixed(2) + '×(' +
            p.tgt.toFixed(3) + ' − ' + p.base.toFixed(3) + ') = ' + p.out.toFixed(3) +
            '  [offset ' + (p.off >= 0 ? '+' : '') + p.off.toFixed(3) + ']';
    var z = p.z, a = Math.min(2, Math.floor(z)), f = z - a;
    var eq = ['T0 = base + clamp(0.6683·raw − base, ±0.10)', 'T1 = min(0.328 + 3.542·(raw − 0.491), 0.75)',
              'T2 = min(0.753 + 3.750·(raw − 0.611), 0.90)', 'T3 = min(0.843 + 0.430·(raw − 0.635), 0.99)'];
    var s2 = 'z=' + z.toFixed(2) + ' → target = (1−' + f.toFixed(2) + ')·T' + a + ' + ' + f.toFixed(2) + '·T' + (a+1) +
             '   |   ' + eq[a] + '  ;  ' + eq[a+1] + '   |   raw=' + p.raw.toFixed(4) + ' ความชัน=' + p.sl.toFixed(4) + '/นาที' +
             (p.eqUsed ? ('   |   จุดสมดุลที่ทำนาย≈' + (typeof p.eqRaw === 'number' ? p.eqRaw.toFixed(4) : '–') + ' (ใช้ช่วยตัดสินโซน)') : '');
    return s + '\n' + s2;
  }
  function drawOffsetChart(cvArg, theme){
    var cv = cvArg || byId('offChart'); if(!cv) return null;
    theme = theme || 'dark';
    var isFormal = (theme === 'formal');
    var COL = isFormal ? {
      bg:'#ffffff', ink:'#111111', muted:'#555555', grid:'#bdbdbd', faint:'rgba(0,0,0,0.06)',
      awTick:'#111111', timeTick:'#555555', ref:'#b3261e', raw:'rgba(179,107,0,0.65)', base:'#777777',
      tgt:'#e65100', out:'#0b3d91', hguide:'rgba(11,61,145,0.55)', offV:'#b3261e', errV:'#1b7f3b',
      boxBg:'rgba(0,0,0,0.04)', boxBorder:'#bdbdbd', boxText:'#111111',
      bandFill:'rgba(179,38,30,0.06)', zeroDash:'rgba(0,0,0,0.25)', offLine:'#b3261e', axisLabel:'#111111', font:'sans-serif'
    } : {
      bg:'#0e1822', ink:'#eaf2f8', muted:'#8fa3b5', grid:'#28394a', faint:'rgba(255,255,255,0.05)',
      awTick:'#38d9c9', timeTick:'#8fa3b5', ref:'#ffd166', raw:'#ffd166', base:'#8fa3b5',
      tgt:'#ff9f5a', out:'#38a9ff', hguide:'rgba(56,169,255,0.75)', offV:'#ff5fa2', errV:'#3ddc84',
      boxBg:'rgba(6,12,18,0.82)', boxBorder:'#28394a', boxText:'#dce6ef',
      bandFill:'rgba(255,95,162,0.07)', zeroDash:'rgba(255,255,255,0.35)', offLine:'#ff5fa2', axisLabel:'#c3d1de', font:'sans-serif'
    };
    var c = cv.getContext('2d'), W = cv.width, H = cv.height;
    c.clearRect(0,0,W,H); c.fillStyle = COL.bg; c.fillRect(0,0,W,H);
    var L = 74, R = 30, T1 = 34, H1 = 430, T2 = 520, H2 = 110, PW = W - L - R;
    c.textAlign = 'left'; c.fillStyle = COL.ink; c.font = '700 17px ' + COL.font;
    c.fillText('กราฟ Offset: aw ก่อน/หลังปรับ, เป้าหมาย, offset และ error ตามเวลา', L, 22);
    if(!offLog.length){
      c.fillStyle = COL.muted; c.textAlign = 'center'; c.font = '16px ' + COL.font;
      c.fillText('ยังไม่มีข้อมูล — เริ่มวัดที่ตัวเครื่อง/กด "เริ่มบันทึกกราฟ" ระบบจะเก็บให้เอง', L + PW/2, T1 + H1/2);
      return cv;
    }
    var pts = offLog, t0 = pts[0].t, tMax = Math.max(1, pts[pts.length-1].t - t0);
    var refV = parseFloat((byId('offRefInput').value || '').replace(',', '.')); var hasRef = !isNaN(refV);
    var lo = 0, hi = 1;
    if(!byId('offZoomToggle') || byId('offZoomToggle').checked){
      lo = 9, hi = -9;
      pts.forEach(function(p){ [p.out, p.base, p.tgt].forEach(function(v){ if(typeof v === 'number' && !isNaN(v)){ lo = Math.min(lo, v); hi = Math.max(hi, v); } }); });
      if(hasRef){ lo = Math.min(lo, refV); hi = Math.max(hi, refV); }
      var pad = Math.max(0.03, (hi - lo) * 0.18); lo = Math.max(0, lo - pad); hi = Math.min(1, hi + pad);
      if(hi - lo < 0.1){ var mid = (hi + lo)/2; lo = Math.max(0, mid - 0.05); hi = Math.min(1, lo + 0.1); }
    }
    function X(t){ return L + ((t - t0)/tMax) * PW; }
    function Y(v){ return T1 + H1 - ((v - lo)/(hi - lo)) * H1; }
    // กริด + แกน
    c.font = '13px ' + COL.font; c.strokeStyle = COL.grid; c.lineWidth = 1.5; c.strokeRect(L, T1, PW, H1);
    for(var i=0;i<=5;i++){
      var v = lo + (hi-lo)*i/5, y = Y(v);
      c.strokeStyle = COL.faint; c.beginPath(); c.moveTo(L,y); c.lineTo(L+PW,y); c.stroke();
      c.textAlign = 'right'; c.fillStyle = COL.awTick; c.fillText(v.toFixed(3), L-8, y+4);
    }
    for(var j=0;j<=6;j++){
      var tt = tMax*j/6, x = X(t0+tt);
      c.strokeStyle = COL.faint; c.beginPath(); c.moveTo(x,T1); c.lineTo(x,T1+H1); c.stroke();
      c.textAlign = 'center'; c.fillStyle = COL.timeTick; c.fillText(tt.toFixed(1)+'m', x, T2+H2+22);
    }
    function line(key, color, width, dash){
      c.setLineDash(dash || []); c.strokeStyle = color; c.lineWidth = width; c.beginPath();
      var started = false;
      pts.forEach(function(p){ var v = p[key]; if(typeof v !== 'number' || isNaN(v)) return; var x = X(p.t), y = Y(v);
        if(!started){ c.moveTo(x,y); started = true; } else c.lineTo(x,y); });
      c.stroke(); c.setLineDash([]);
    }
    if(hasRef){
      c.setLineDash([8,4]); c.strokeStyle = COL.ref; c.lineWidth = 1.6; c.beginPath(); c.moveTo(L,Y(refV)); c.lineTo(L+PW,Y(refV)); c.stroke(); c.setLineDash([]);
      c.textAlign = 'left'; c.fillStyle = COL.ref; c.fillText('ค่าจริง ' + refV.toFixed(3), L+6, Y(refV)-6);
    }
    line('raw', COL.raw, 1.4, [2,3]);          // ค่าดิบ (RH/100) — อยู่คนละสเกลกับ aw จึงเห็นเฉพาะตอนซูมครอบ
    line('base', COL.base, 1.6);
    line('tgt', COL.tgt, 1.6, [6,4]);
    line('out', COL.out, 2.4);
    // ---- จุดกำกับ: ทุก ~60 จุด (ไม่ถี่เกิน) + จุดล่าสุดเสมอ ----
    var marks = [], step = Math.max(60, Math.ceil(pts.length / 5));
    for(var k = step; k < pts.length - 25; k += step) marks.push(k);
    marks.push(pts.length - 1);
    marks.forEach(function(idx, mi){
      var p = pts[idx], x = X(p.t), yb = Y(p.base), yo = Y(p.out), yt = Y(p.tgt);
      var last = (idx === pts.length - 1);
      // เส้นประแนวนอน: บอกระดับ aw หลังปรับ ณ จุดนั้นลากไปทางขวา
      c.setLineDash([4,4]); c.strokeStyle = COL.hguide; c.lineWidth = 1.2;
      c.beginPath(); c.moveTo(x, yo); c.lineTo(Math.min(L+PW, x+110), yo); c.stroke();
      c.setLineDash([]);
      // เส้นตั้ง offset: ตาราง -> หลังปรับ
      c.strokeStyle = COL.offV; c.lineWidth = 2.6; c.beginPath(); c.moveTo(x, yb); c.lineTo(x, yo); c.stroke();
      // เส้นตั้ง error: หลังปรับ -> ค่าจริง (ถ้ากรอก) ไม่งั้น -> เป้าหมายของโซน
      var eTo = hasRef ? refV : p.tgt, eY = Y(eTo), eVal = eTo - p.out;
      c.strokeStyle = COL.errV; c.lineWidth = 2; c.beginPath(); c.moveTo(x+6, yo); c.lineTo(x+6, eY); c.stroke();
      c.lineWidth = 2; c.strokeStyle = COL.offV; c.beginPath(); c.moveTo(x-6, yb); c.lineTo(x+6, yb); c.stroke();
      c.strokeStyle = COL.errV; c.beginPath(); c.moveTo(x, eY); c.lineTo(x+12, eY); c.stroke();
      c.fillStyle = COL.out; c.beginPath(); c.arc(x, yo, 4, 0, 6.283); c.fill();
      // ป้ายตัวเลข
      c.font = '12px ' + COL.font; c.textAlign = 'left';
      var tx = (x > L + PW - 190) ? x - 186 : x + 14, ty = Math.max(T1+14, Math.min(T1+H1-30, Math.min(yb, yo) - 8));
      c.fillStyle = COL.offV; c.fillText('offset ' + (p.off>=0?'+':'') + p.off.toFixed(3) + ' (z=' + p.z.toFixed(2) + ')', tx, ty);
      c.fillStyle = COL.errV; c.fillText('error ' + (eVal>=0?'+':'') + eVal.toFixed(3) + (hasRef && refV ? ' (' + (eVal/refV*100).toFixed(1) + '%)' : ''), tx, ty + 14);
      if(last){
        // กล่องสูตร ณ จุดล่าสุด
        var lines = offFormulaText(p).split('\n'); lines.unshift(offZoneName(p.z));
        var bw = Math.min(PW - 20, 760), bh = 16 * 3 + 12;
        c.fillStyle = COL.boxBg; c.fillRect(L + 10, T1 + 8, bw, bh);
        c.strokeStyle = COL.boxBorder; c.lineWidth = 1; c.strokeRect(L + 10, T1 + 8, bw, bh);
        c.fillStyle = COL.boxText; c.font = '12px monospace'; c.textAlign = 'left';
        lines.slice(0,3).forEach(function(s, si){ c.fillText(s.length > 118 ? s.slice(0,118) + '…' : s, L + 18, T1 + 24 + si*16); });
      }
    });
    // ---- กราฟล่าง: offset ตามเวลา ----
    var oMax = 0.12; pts.forEach(function(p){ if(typeof p.off === 'number' && !isNaN(p.off)) oMax = Math.max(oMax, Math.abs(p.off) * 1.2); });
    function Y2(v){ return T2 + H2/2 - (v/oMax) * (H2/2); }
    c.strokeStyle = COL.grid; c.lineWidth = 1.5; c.strokeRect(L, T2, PW, H2);
    c.fillStyle = COL.bandFill; c.fillRect(L, Y2(0.10), PW, Y2(-0.10) - Y2(0.10));
    c.setLineDash([3,3]); c.strokeStyle = COL.zeroDash; c.beginPath(); c.moveTo(L, Y2(0)); c.lineTo(L+PW, Y2(0)); c.stroke(); c.setLineDash([]);
    c.font = '12px ' + COL.font; c.textAlign = 'right'; c.fillStyle = COL.offLine;
    c.fillText('+' + oMax.toFixed(2), L-8, T2+10); c.fillText('0', L-8, Y2(0)+4); c.fillText('-' + oMax.toFixed(2), L-8, T2+H2);
    c.strokeStyle = COL.offLine; c.lineWidth = 2; c.beginPath();
    pts.forEach(function(p, i){ var x = X(p.t), y = Y2(p.off || 0); if(i===0) c.moveTo(x,y); else c.lineTo(x,y); });
    c.stroke();
    c.textAlign = 'left'; c.fillStyle = COL.axisLabel; c.font = '600 13px ' + COL.font;
    c.fillText('Offset (aw)', L + 6, T2 + 16);
    return cv;
  }

  function offUpdateReadouts(){
    if(!offLog.length) return;
    var p = offLog[offLog.length-1];
    byId('offNowVal').textContent = (p.off>=0?'+':'') + p.off.toFixed(3);
    byId('offZoneVal').textContent = p.z.toFixed(2) + ' — ' + offZoneName(p.z).split(':')[0];
    byId('offSlopeVal').textContent = p.sl.toFixed(4) + (p.eqUsed ? ('  (จุดสมดุลที่ทำนาย≈' + p.eqRaw.toFixed(4) + ')') : '');
    byId('offWVal').textContent = p.w.toFixed(2);
    byId('offFormula').textContent = 'สูตรตอนนี้:\n' + offFormulaText(p);
    byId('offFormula').style.whiteSpace = 'pre-wrap';
  }
  function offCsv(){
    var rows = ['t_min,raw,aw_table,aw_out,offset,target,z,w,slope_raw_per_min,eq_raw_predicted,eq_used'];
    offLog.forEach(function(p){ rows.push([p.t.toFixed(3),p.raw,p.base,p.out,p.off,p.tgt,p.z,p.w,p.sl,
      (p.eqRaw===null?'':p.eqRaw),p.eqUsed?1:0].join(',')); });
    var a = document.createElement('a'); a.href = URL.createObjectURL(new Blob([rows.join('\n')], {type:'text/csv'}));
    a.download = 'offset_log.csv'; a.click();
  }

  // ============================================================
  //  v-trend-offset: เส้นโค้งคาลิเบรตสด + ตารางสรุป R²/offset/error ต่อโซน
  //  (รูปแบบเดียวกับกราฟถดถอยเชิงเส้น + ตาราง R²/LOD/LOQ ที่ใช้รายงานผลงานวิจัย)
  // ============================================================
  function offLinRegress(pts){    // pts: [{x,y}] -> least squares y = m*x + c, พร้อม R²
    var n = pts.length;
    if(n < 2) return null;
    var sx=0, sy=0; pts.forEach(function(p){ sx+=p.x; sy+=p.y; });
    var mx = sx/n, my = sy/n;
    var sxy=0, sxx=0, syy=0;
    pts.forEach(function(p){ var dx=p.x-mx, dy=p.y-my; sxy+=dx*dy; sxx+=dx*dx; syy+=dy*dy; });
    if(sxx < 1e-9) return null;
    var m = sxy/sxx, c = my - m*mx;
    var ssRes = 0; pts.forEach(function(p){ var e = p.y - (m*p.x+c); ssRes += e*e; });
    var r2 = (syy > 1e-9) ? (1 - ssRes/syy) : 1;
    return { m:m, c:c, r2:r2, n:n };
  }
  function offZoneOf(z){ return z < 0.5 ? 0 : (z < 1.5 ? 1 : (z < 2.5 ? 2 : 3)); }   // ปัดเป็นโซนจำนวนเต็มไว้จัดกลุ่ม/สี
  function offPointsFor(sel){
    return offLog.filter(function(p){ return typeof p.raw === 'number' && typeof p.out === 'number' && !isNaN(p.raw) && !isNaN(p.out) &&
      (sel === 'all' || offZoneOf(p.z) === parseInt(sel,10)); });
  }
  function offBinnedPoints(pts){    // เฉลี่ย aw ต่อช่วง raw กว้าง 0.005 (จุดดำในกราฟ เหมือนจุดต่อความเข้มข้นในงานวิจัย)
    var bins = {};
    pts.forEach(function(p){ var k = Math.round(p.raw/0.005); (bins[k] = bins[k]||[]).push(p.out); });
    var out = [];
    Object.keys(bins).forEach(function(k){ var arr = bins[k]; out.push({ x: parseFloat(k)*0.005, y: arr.reduce(function(a,b){return a+b;},0)/arr.length }); });
    out.sort(function(a,b){ return a.x-b.x; });
    return out;
  }
  function drawOffCalCurve(cvArg, theme){
    var cv = cvArg || byId('offCalCurve'); if(!cv) return null;
    theme = theme || 'dark';
    var isFormal = (theme === 'formal');
    var COL = isFormal ? {
      bg:'#ffffff', ink:'#111111', muted:'#555555', faint:'rgba(0,0,0,0.06)',
      grid:'#bdbdbd', axisLabel:'#111111', fit:'#b3261e', ptFill:'#ffffff', ptStroke:'#111111', font:'sans-serif'
    } : {
      bg:'#0e1822', ink:'#eaf2f8', muted:'#8fa3b5', faint:'rgba(255,255,255,0.05)',
      grid:'#28394a', axisLabel:'#c3d1de', fit:'#ff5f5f', ptFill:'#0e1822', ptStroke:'#eaf2f8', font:'sans-serif'
    };
    var c = cv.getContext('2d'), W = cv.width, H = cv.height;
    c.clearRect(0,0,W,H); c.fillStyle = COL.bg; c.fillRect(0,0,W,H);
    var L=64, R=20, T=20, Bm=48, PW=W-L-R, PH=H-T-Bm;
    var sel = byId('offCalZoneSel') ? byId('offCalZoneSel').value : 'all';
    var raw = offPointsFor(sel).map(function(p){ return {x:p.raw, y:p.out}; });
    var bins = offBinnedPoints(raw);
    c.strokeStyle = COL.grid; c.lineWidth = 1.5; c.strokeRect(L,T,PW,PH);
    if(bins.length < 2){
      c.fillStyle = COL.muted; c.textAlign = 'center'; c.font = '14px ' + COL.font;
      c.fillText('ยังไม่มีข้อมูลพอสำหรับ fit เส้นตรง (ต้องการอย่างน้อย 2 ช่วง raw)', L+PW/2, T+PH/2);
      return cv;
    }
    var xMin = bins[0].x, xMax = bins[bins.length-1].x;
    var yMin = Math.min.apply(null, bins.map(function(p){return p.y;})), yMax = Math.max.apply(null, bins.map(function(p){return p.y;}));
    if(xMax-xMin < 0.01){ xMin -= 0.02; xMax += 0.02; }
    if(yMax-yMin < 0.02){ var ym=(yMin+yMax)/2; yMin=ym-0.03; yMax=ym+0.03; }
    var padY = (yMax-yMin)*0.15; yMin -= padY; yMax += padY;
    function X(v){ return L + (v-xMin)/(xMax-xMin)*PW; }
    function Y(v){ return T + PH - (v-yMin)/(yMax-yMin)*PH; }
    c.font='12px ' + COL.font; c.fillStyle=COL.muted; c.textAlign='center';
    for(var i=0;i<=5;i++){ var xv=xMin+(xMax-xMin)*i/5; var x=X(xv);
      c.strokeStyle=COL.faint; c.beginPath(); c.moveTo(x,T); c.lineTo(x,T+PH); c.stroke();
      c.fillText(xv.toFixed(3), x, T+PH+16); }
    c.textAlign='right';
    for(var j=0;j<=5;j++){ var yv=yMin+(yMax-yMin)*j/5; var y=Y(yv);
      c.strokeStyle=COL.faint; c.beginPath(); c.moveTo(L,y); c.lineTo(L+PW,y); c.stroke();
      c.fillText(yv.toFixed(3), L-8, y+4); }
    c.textAlign='center'; c.fillStyle=COL.axisLabel; c.font='600 12px ' + COL.font;
    c.fillText('ค่าดิบ raw', L+PW/2, H-8);
    c.save(); c.translate(14, T+PH/2); c.rotate(-Math.PI/2); c.fillText('aw หลังปรับ', 0, 0); c.restore();
    var reg = offLinRegress(bins);
    if(reg){
      c.strokeStyle = COL.fit; c.lineWidth = 2; c.beginPath();
      c.moveTo(X(xMin), Y(reg.m*xMin+reg.c)); c.lineTo(X(xMax), Y(reg.m*xMax+reg.c)); c.stroke();
      c.textAlign='left'; c.fillStyle=COL.ink; c.font='600 13px ' + COL.font;
      c.fillText('aw = ' + reg.m.toFixed(3) + ' · raw + ' + (reg.c>=0?'':'') + reg.c.toFixed(3), L+14, T+20);
      c.fillText('R² = ' + reg.r2.toFixed(4) + '   (n=' + reg.n + ' ช่วง)', L+14, T+38);
    }
    c.fillStyle = COL.ptFill; c.strokeStyle = COL.ptStroke; c.lineWidth = 1.4;
    bins.forEach(function(p){ var x=X(p.x), y=Y(p.y); c.beginPath(); c.rect(x-4,y-4,8,8); c.fill(); c.stroke(); });
    return cv;
  }

  function offUpdateStatsTable(){
    var tb = byId('offStatsBody'); if(!tb) return;
    var refV = parseFloat((byId('offRefInput').value || '').replace(',', '.')); var hasRef = !isNaN(refV);
    var names = ['0 — CaCl2/MgCl2', '1 — MgCl2→NaCl', '2 — NaCl→KCl', '3 — KCl→น้ำบริสุทธิ์'];
    var rowsHtml = '';
    var any = false;
    for(var z=0; z<4; z++){
      var pts = offLog.filter(function(p){ return offZoneOf(p.z) === z; });
      if(!pts.length){ rowsHtml += '<tr><td>' + names[z] + '</td><td colspan="6" style="color:#8fa3b5;">ยังไม่มีข้อมูล</td></tr>'; continue; }
      any = true;
      var rawArr = pts.map(function(p){ return p.raw; });
      var offArr = pts.map(function(p){ return p.off; });
      var reg = offLinRegress(pts.map(function(p){ return {x:p.raw, y:p.out}; }));
      var errArr = pts.map(function(p){ var t = hasRef ? refV : p.tgt; return Math.abs(p.out - t); });
      var meanErr = errArr.reduce(function(a,b){return a+b;},0) / errArr.length;
      rowsHtml += '<tr><td>' + names[z] + '</td>' +
        '<td>' + Math.min.apply(null,rawArr).toFixed(4) + ' – ' + Math.max.apply(null,rawArr).toFixed(4) + '</td>' +
        '<td>' + pts.length + '</td>' +
        '<td>' + (reg ? reg.r2.toFixed(4) : '–') + '</td>' +
        '<td>' + Math.min.apply(null,offArr).toFixed(3) + '</td>' +
        '<td>' + Math.max.apply(null,offArr).toFixed(3) + '</td>' +
        '<td>' + meanErr.toFixed(3) + (hasRef ? '' : ' (เทียบ target)') + '</td></tr>';
    }
    tb.innerHTML = any ? rowsHtml : '<tr><td colspan="7" style="text-align:center; color:#8fa3b5;">ยังไม่มีข้อมูล</td></tr>';
  }

  // ============================================================
  //  รอบทดสอบที่บันทึกไว้เพื่อเปรียบเทียบ
  // ============================================================
  function runStats(run){
    var pts = run.points;
    var last = pts.length ? pts[pts.length-1] : null;
    var raw = last ? last.raw : null;
    var cal = last ? last.aw : null;
    var ref = run.refAw;
    // v-cal-fix: เดิมไม่กัน ref===0 ทำให้ตารางขึ้น "Infinity%" (หารด้วยศูนย์) ตอนใช้ค่าจริง=0.000
    // (เช่น ซองดูดความชื้น) — ตอนนี้ ref=0 จะถือว่าคำนวณ %error ไม่ได้ (แสดง "–" แทน เหมือนจุดอื่นในไฟล์นี้)
    var errRaw = (typeof ref === 'number' && !isNaN(ref) && ref !== 0 && raw !== null) ? ((raw-ref)/ref*100) : null;
    var errCal = (typeof ref === 'number' && !isNaN(ref) && ref !== 0 && cal !== null) ? ((cal-ref)/ref*100) : null;
    return { ref:ref, raw:raw, cal:cal, errRaw:errRaw, errCal:errCal };
  }

  function renderRunList(){
    var wrap = byId('runList');
    wrap.innerHTML = '';
    byId('emptyNote').style.display = savedRuns.length ? 'none' : 'block';
    savedRuns.forEach(function(run){
      var row = document.createElement('div');
      row.className = 'run-row';
      row.innerHTML =
        '<input type="checkbox" class="visToggle" data-id="' + run.id + '"' + (run.visible?' checked':'') + '>' +
        '<span class="swatch" style="background:' + run.color + '"></span>' +
        '<span class="rname">' + escapeHtml(run.name) + '</span>' +
        (run.pred && typeof run.pred.eqAw === 'number'
          ? '<span class="predBadge">ทำนาย: ' + (run.pred.confident?'':'~') + run.pred.eqAw.toFixed(4) + '</span>'
          : '') +
        '<input type="number" step="0.001" placeholder="ค่าจริง" class="refInput" data-id="' + run.id + '" value="' + (typeof run.refAw==='number'?run.refAw:'') + '">' +
        '<button class="rename" data-id="' + run.id + '">แก้ไขชื่อ</button>' +
        '<button class="del" data-id="' + run.id + '">ลบ</button>';
      wrap.appendChild(row);
    });
    wrap.querySelectorAll('.visToggle').forEach(function(cb){
      cb.addEventListener('change', function(){
        var run = savedRuns.find(function(r){ return r.id === cb.dataset.id; });
        if(run){ run.visible = cb.checked; persistRuns(); drawChart(); drawComparePredictChart(); }
      });
    });
    wrap.querySelectorAll('.refInput').forEach(function(inp){
      inp.addEventListener('change', function(){
        var run = savedRuns.find(function(r){ return r.id === inp.dataset.id; });
        if(run){
          var val = parseFloat(inp.value);
          run.refAw = isNaN(val) ? null : val;
          persistRuns(); renderStatsTable();
        }
      });
    });
    wrap.querySelectorAll('.rename').forEach(function(btn){
      btn.addEventListener('click', function(){
        var run = savedRuns.find(function(r){ return r.id === btn.dataset.id; });
        if(!run) return;
        var newName = prompt('แก้ไขชื่อกราฟ:', run.name);
        if(newName === null) return;
        newName = newName.trim();
        if(!newName) return;
        run.name = newName;
        persistRuns(); renderRunList(); drawChart(); drawComparePredictChart();
      });
    });
    wrap.querySelectorAll('.del').forEach(function(btn){
      btn.addEventListener('click', function(){
        savedRuns = savedRuns.filter(function(r){ return r.id !== btn.dataset.id; });
        persistRuns(); renderRunList(); drawChart(); renderStatsTable(); drawComparePredictChart();
      });
    });
    renderStatsTable();
    drawComparePredictChart(); // v-pro: อัปเดตกราฟเปรียบเทียบค่าทำนายให้ตรงกับรายการ/ชื่อล่าสุดเสมอ
    refreshReportSourceOptions(); // v-report: อัปเดตตัวเลือก "ข้อมูลที่จะใช้ออกรายงาน" ให้ตรงกับรอบที่บันทึกไว้ล่าสุดเสมอ
  }

  // v-report: เติมตัวเลือกในดรอปดาวน์ #rptSource ด้วย "ค่าปัจจุบัน (Live)" + รอบทดสอบที่บันทึกไว้ทุกอัน
  // เรียกทุกครั้งที่ savedRuns เปลี่ยน (บันทึกใหม่/ลบ/ล้างทั้งหมด) ผ่าน renderRunList() ด้านบน
  function refreshReportSourceOptions(){
    var sel = byId('rptSource');
    if(!sel) return;
    var prevVal = sel.value;
    sel.innerHTML = '<option value="live">ค่าปัจจุบัน (Live)</option>' +
      savedRuns.map(function(r){ return '<option value="' + r.id + '">' + escapeHtml(r.name) + '</option>'; }).join('');
    if(savedRuns.some(function(r){ return r.id === prevVal; }) || prevVal === 'live') sel.value = prevVal;
  }

  function renderStatsTable(){
    var rows = savedRuns.filter(function(r){ return typeof r.refAw === 'number' && !isNaN(r.refAw); });
    var table = byId('statsTable'), body = byId('statsBody');
    if(!rows.length){ table.style.display = 'none'; body.innerHTML = ''; return; }
    table.style.display = 'table';
    body.innerHTML = rows.map(function(r){
      var st = runStats(r);
      return '<tr><td>' + escapeHtml(r.name) + '</td><td>' + st.ref.toFixed(3) + '</td>' +
        '<td>' + (st.raw!==null?st.raw.toFixed(4):'–') + '</td>' +
        '<td>' + (st.cal!==null?st.cal.toFixed(3):'–') + '</td>' +
        '<td>' + (st.errRaw!==null?st.errRaw.toFixed(2)+'%':'–') + '</td>' +
        '<td>' + (st.errCal!==null?st.errCal.toFixed(2)+'%':'–') + '</td></tr>';
    }).join('');
  }

  function escapeHtml(s){
    return String(s).replace(/[&<>"']/g, function(c){
      return ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[c];
    });
  }

  // ============================================================
  //  เมทาดาทาอุปกรณ์ (/info) — เรียกครั้งเดียวตอนโหลดหน้า
  // ============================================================
  function loadDeviceInfo(){
    fetch('/info').then(function(r){ return r.json(); }).then(function(d){
      deviceInfoCache = d; // v-report: เก็บไว้ใช้ตอนสร้างใบรายงานผล
      valueMode = d.valueMode || 'CALIBRATED';
      byId('deviceSub').textContent = d.deviceName ? (d.deviceName + ' — Water Activity Research Console') : 'Water Activity Research Console';
      var badge = byId('modeBadge');
      badge.style.display = 'inline-block';
      badge.textContent = valueMode;
      badge.style.color = (valueMode === 'RAW') ? '#ffcc4d' : '#38d9c9';
      badge.style.borderColor = (valueMode === 'RAW') ? 'rgba(255,204,77,.4)' : 'rgba(56,217,201,.35)';
      badge.style.background = (valueMode === 'RAW') ? 'rgba(255,204,77,.12)' : 'rgba(56,217,201,.12)';

      byId('fwStat').textContent = d.fwVersion || '–';
      byId('rhSensorStat').textContent = d.humiditySensor || '–';
      byId('tempSensorStat').textContent = d.tempSensor || '–';
      byId('opInput').value = d.operator || '';

      if(valueMode !== 'RAW'){
        byId('calCard').style.display = 'block';
        renderCalRows(d.calPoints || []);
      }
    }).catch(function(e){ console.error('/info โหลดไม่สำเร็จ', e); });
  }

  function renderCalRows(points){
    var wrap = byId('calRows');
    wrap.innerHTML = points.map(function(p, i){
      return '<div class="frow"><label>จุดที่ ' + (i+1) + '</label>' +
        '<input type="text" class="calRaw" data-i="' + i + '" placeholder="raw (0-1)" value="' + p.raw.toFixed(4) + '" style="max-width:110px;">' +
        '<input type="text" class="calAw" data-i="' + i + '" placeholder="aw (0-1)" value="' + p.aw.toFixed(4) + '" style="max-width:110px;">' +
        '</div>';
    }).join('');
    renderCalSlopeBox();  // v-slope-live: อัปเดตช่องสโลปทันทีทุกครั้งที่ตารางจุดถูกวาดใหม่ (โหลด/รีเซ็ต/ใส่ค่าที่เสนอ/จากตัวช่วยอัตโนมัติ)
  }

  // ============================================================
  //  v-slope-live: ช่องใหม่ — โชว์ "จุดที่ยืด/หดสเกล" สดจากค่าที่กำลังพิมพ์อยู่ในตาราง (ยังไม่ต้องกดบันทึก)
  // ============================================================
  // อ่านค่า raw/aw ปัจจุบันจากช่อง input ตรงๆ (ไม่ใช่จากค่าที่เคยโหลดมา) แล้วคำนวณความชัน (slope = Δaw/Δraw) ของ
  // แต่ละช่วงระหว่างจุดที่ i กับ i+1 — ถ้าความชันห่างจาก ×1 มาก แปลว่ากำลัง "ยืด" (>1) หรือ "หด" (<1) สเกลแรงในช่วงนั้น
  // ซึ่งปกติเซนเซอร์ SHT45 ที่ผ่านการคาลิเบรตโรงงานมาแล้วไม่ควรต้องยืด/หดเกิน ×0.75–×1.3 (เกณฑ์เดียวกับที่ตัวช่วย
  // เสนอจุดคาลิเบรตอัตโนมัติใช้เตือนอยู่แล้ว ดู buildCalProposal ด้านบน) ถ้าเกินช่วงนี้มักแปลว่าปัญหาไม่ได้อยู่ที่ตัวเซนเซอร์
  // (เช่น ห้องวัดยังไม่สมดุล/ไม่ปิดผนึกตอนอ่านค่าอ้างอิง) ไม่ใช่ให้ยืด/หดสเกลกลบไป — โชว์ตรงนี้จะเห็นได้ทันทีตั้งแต่ตอนจูน
  // ไม่ต้องรอกดบันทึกแล้วมาลุ้นทีหลัง เพิ่มความแม่นยำของการจูนได้มาก ทั้งจุดคาลิเบรตปกติและจุดที่ใช้ร่วมกับตัวคูณชดเชยอุณหภูมิ
  function renderCalSlopeBox(){
    var box = byId('calSlopeBox'); if(!box) return;
    var raws = document.querySelectorAll('.calRaw'), aws = document.querySelectorAll('.calAw');
    if(!raws.length){ box.innerHTML = ''; return; }
    var pts = [];
    for(var i = 0; i < raws.length; i++){
      pts.push({ raw: parseFloat(raws[i].value), aw: parseFloat(aws[i].value) });
    }
    var bad = pts.some(function(p){ return isNaN(p.raw) || isNaN(p.aw); });
    if(bad){ box.innerHTML = '<span style="color:#ffb4b4;">กรอกตัวเลขให้ครบทุกช่องก่อน ถึงจะคำนวณสโลปแต่ละช่วงให้ดูได้</span>'; return; }

    // ต้องเรียง raw จากน้อยไปมากเสมอ (ตรงกับ isCalPointsMonotonic() ฝั่งเฟิร์มแวร์) ไม่งั้น applyCal() จะพัง/หารด้วยศูนย์
    var monotonic = true;
    for(var m = 1; m < pts.length; m++){ if(pts[m].raw <= pts[m - 1].raw){ monotonic = false; break; } }

    var rows = '';
    for(var j = 1; j < pts.length; j++){
      var dRaw = pts[j].raw - pts[j - 1].raw;
      var slope = dRaw !== 0 ? (pts[j].aw - pts[j - 1].aw) / dRaw : NaN;
      var tag, color;
      if(isNaN(slope) || !isFinite(slope)){ tag = 'คำนวณไม่ได้ (raw ซ้ำกัน)'; color = '#ffb4b4'; }
      else if(j > 1 && (slope > 1.3 || slope < 0.75)){ tag = (slope > 1 ? 'ยืดสเกลแรง' : 'หดสเกลแรง') + ' — น่าสงสัยว่าไม่ใช่ที่ตัวเซนเซอร์'; color = '#ffb4b4'; }
      else if(slope > 1.05){ tag = 'ยืดสเกลเล็กน้อย'; color = '#ffd27a'; }
      else if(slope < 0.95){ tag = 'หดสเกลเล็กน้อย'; color = '#ffd27a'; }
      else { tag = 'ปกติ'; color = '#7be0a5'; }
      rows += '<tr><td>จุด ' + j + ' → ' + (j + 1) + '</td><td>raw ' + pts[j-1].raw.toFixed(4) + '→' + pts[j].raw.toFixed(4) + '</td>' +
              '<td>×' + (isFinite(slope) ? slope.toFixed(3) : '–') + '</td><td style="color:' + color + ';">' + tag + '</td></tr>';
    }
    var gfLine = '';
    if(lastData && typeof lastData.gf === 'number' && valueMode !== 'RAW'){
      gfLine = '<div style="margin-top:6px;">ตัวคูณชดเชยอุณหภูมิที่ใช้งานจริงตอนนี้ (แยกจากตารางด้านบน คูณต่อกัน): ×' + lastData.gf.toFixed(4) +
               (typeof lastData.shtT === 'number' && lastData.shtT > -50 ? ' (ที่ชิป ' + lastData.shtT.toFixed(1) + ' °C)' : '') + '</div>';
    }
    box.innerHTML = '<table style="margin-top:4px;"><thead><tr><th>ช่วง</th><th>raw</th><th>สโลป (Δaw/Δraw)</th><th>สถานะ</th></tr></thead><tbody>' + rows + '</tbody></table>' +
      (!monotonic ? '<div style="color:#ffb4b4;margin-top:6px;">⚠️ ค่า raw ต้องเรียงจากน้อยไปมากทุกจุด ไม่งั้นบันทึกไม่ผ่าน (isCalPointsMonotonic)</div>' : '') +
      gfLine;
  }

  // ============================================================
  //  v14: ผู้ช่วยคาลิเบรต — สร้างจุดคาลิเบรต (piecewise-linear, CAL_POINTS_TOTAL จุด) จากรอบที่บันทึกไว้พร้อมค่าอ้างอิง (ช่อง "ค่าจริง")
  // ============================================================
  // หลักการ: (1) ค่า raw ของแต่ละรอบ = มัธยฐานของช่วงท้ายรอบ (15% สุดท้าย) (2) รวมรอบที่ค่าอ้างอิงเท่ากันเป็นกลุ่มเดียว
  // (3) จุดคาลิเบรต = (0,0) + กลุ่มอ้างอิง 3 ระดับ  (raw เฉลี่ยของกลุ่ม -> ค่าอ้างอิง / ตัวคูณชดเชยอุณหภูมิเฉลี่ย)
  // ผู้ช่วย "ไม่เดา" — ถ้าข้อมูลขัดกันเอง (ค่าอ้างอิงสูงกว่าแต่อ่านได้ต่ำกว่า) ซ้ำรอบเดียวกันกระจายมาก หรือความชันเพี้ยนจากเซนเซอร์จริง
  // จะแจ้งเตือน/ปฏิเสธ เพราะปัญหามักอยู่ที่ห้องวัดยังไม่ถึงสมดุล/ไม่ปิดผนึก ไม่ใช่ตัวเซนเซอร์ (SHT35 ผ่านการคาลิเบรตจากโรงงานแล้ว)
  // ==CALASSIST_BEGIN==
  // v-multi-sample: ต้องตรงกับ const int CAL_POINTS_COUNT ในโค้ดเฟิร์มแวร์ (ใกล้ applyCal()) เสมอ — ใช้ตัวนี้กำหนดว่า
  // ผู้ช่วยคาลิเบรตต้องการค่าอ้างอิงกี่ระดับ (CAL_POINTS_TOTAL - 1 ระดับ + จุด (0,0) อีก 1 จุด = ครบตาราง)
  // v15: 6 -> 9 จุด (8 ระดับอ้างอิง) ให้ตรงกับ CAL_POINTS_COUNT ฝั่งเฟิร์มแวร์ — เส้นคาลิเบรตละเอียดขึ้น
  var CAL_POINTS_TOTAL = 27;
  function calMedian(a){ var b = a.slice().sort(function(x, y){ return x - y; }), n = b.length; return n ? (n % 2 ? b[(n - 1) / 2] : (b[n / 2 - 1] + b[n / 2]) / 2) : NaN; }
  function calMean(a){ return a.length ? a.reduce(function(s, v){ return s + v; }, 0) / a.length : NaN; }
  function calSd(a){ if(a.length < 2) return 0; var m = calMean(a); return Math.sqrt(a.reduce(function(s, v){ return s + (v - m) * (v - m); }, 0) / (a.length - 1)); }
  function calSlope(pts, key){   // ความชันของ key เทียบเวลา (ต่อนาที) ด้วย least squares
    var n = pts.length; if(n < 3) return 0;
    var mt = calMean(pts.map(function(p){ return p.t; })), my = calMean(pts.map(function(p){ return p[key]; })), sxx = 0, sxy = 0;
    pts.forEach(function(p){ sxx += (p.t - mt) * (p.t - mt); sxy += (p.t - mt) * (p[key] - my); });
    return sxx > 0 ? sxy / sxx : 0;
  }

  function calRunFinal(run){
    var pts = (run.points || []).filter(function(p){ return typeof p.raw === 'number' && !isNaN(p.raw); });
    var n = pts.length;
    if(n < 30) return null;
    var tail = pts.slice(-Math.max(20, Math.floor(n * 0.15)));
    var last = pts.slice(-Math.max(30, Math.floor(n * 0.3)));
    var temps = tail.map(function(p){ return p.temp; }).filter(function(v){ return typeof v === 'number' && v > -50; });
    return {
      name: run.name, ref: run.refAw,
      raw: calMedian(tail.map(function(p){ return p.raw; })),
      gf: calMean(tail.map(function(p){ return (typeof p.gf === 'number' && p.gf > 0) ? p.gf : 1; })),
      temp: temps.length ? calMean(temps) : NaN,
      durMin: Math.max(0, pts[n - 1].t - pts[0].t),
      driftPerMin: calSlope(last, 'raw'),
      n: n
    };
  }

  // runs: savedRuns  |  คืน {ok, reason, groups, points, warnings, segments}
  function buildCalProposal(runs){
    var finals = [], skipped = [];
    (runs || []).forEach(function(r){
      if(typeof r.refAw !== 'number' || isNaN(r.refAw) || r.refAw <= 0){ return; }   // ไม่ได้ใส่ค่าอ้างอิง (หรือ 0) = ไม่ใช้
      var f = calRunFinal(r);
      if(f) finals.push(f); else skipped.push(r.name);
    });
    var warnings = [];
    if(skipped.length) warnings.push('ข้ามรอบที่มีจุดข้อมูลน้อยกว่า 30 จุด: ' + skipped.join(', '));
    var map = {};
    finals.forEach(function(f){ var k = f.ref.toFixed(3); (map[k] = map[k] || []).push(f); });
    var groups = Object.keys(map).map(function(k){
      var a = map[k];
      return { ref: parseFloat(k), n: a.length, names: a.map(function(x){ return x.name; }),
               raw: calMean(a.map(function(x){ return x.raw; })), sd: calSd(a.map(function(x){ return x.raw; })),
               gf: calMean(a.map(function(x){ return x.gf; })), temp: calMean(a.map(function(x){ return x.temp; })),
               minDur: Math.min.apply(null, a.map(function(x){ return x.durMin; })),
               maxDrift: Math.max.apply(null, a.map(function(x){ return Math.abs(x.driftPerMin); })),
               tempSpread: (function(){ var t = a.map(function(x){ return x.temp; }).filter(function(v){ return !isNaN(v); }); return t.length > 1 ? Math.max.apply(null, t) - Math.min.apply(null, t) : 0; })() };
    }).sort(function(a, b){ return a.ref - b.ref; });

    var neededLevels = CAL_POINTS_TOTAL - 1; // จุด (0,0) นับรวมอยู่แล้วนอกเหนือจากระดับค่าอ้างอิงที่วัดจริง
    if(groups.length < neededLevels)
      return { ok:false, groups:groups, warnings:warnings,
               reason:'ต้องมีค่าอ้างอิงที่ต่างกันอย่างน้อย ' + neededLevels + ' ระดับ (ตอนนี้มี ' + groups.length + ' ระดับ) เช่น LiCl 0.113 · CH₃COOK 0.225 · MgCl₂ 0.328 · K₂CO₃ 0.432 · NaBr 0.577 · NaCl 0.753 · KCl 0.843 · K₂SO₄ 0.973 ที่ 25 °C — ใส่ค่าในช่อง "ค่าจริง" ของแต่ละรอบก่อน (ยิ่งใช้หลายระดับ เส้นคาลิเบรตยิ่งแม่นและเสถียรขึ้น)' };

    // ข้อมูลขัดแย้ง: ค่าอ้างอิงสูงขึ้นแต่ raw ไม่สูงขึ้นตาม
    var conflicts = [];
    for(var i = 1; i < groups.length; i++){
      if(groups[i].raw <= groups[i - 1].raw + 0.005)
        conflicts.push('ค่าอ้างอิง ' + groups[i - 1].ref.toFixed(3) + ' อ่านได้ raw ' + groups[i - 1].raw.toFixed(3) + ' แต่ค่าอ้างอิง ' + groups[i].ref.toFixed(3) + ' อ่านได้ raw ' + groups[i].raw.toFixed(3) + ' (ไม่สูงขึ้นตาม)');
    }
    if(conflicts.length)
      return { ok:false, groups:groups, warnings:warnings, conflicts:conflicts,
               reason:'ข้อมูลขัดแย้งกันเอง จึงไม่สร้างตารางให้ — เซนเซอร์แยกตัวอย่างเหล่านี้ไม่ได้ในสภาพการวัดครั้งนี้ (มักเกิดจากห้องวัดยังไม่ถึงสมดุล ไม่ปิดผนึก หรืออุณหภูมิต่างกัน ไม่ใช่ความผิดของตัวเซนเซอร์)' };

    var chosen = groups;
    if(groups.length > neededLevels){   // เลือก neededLevels ระดับ กระจายเท่า ๆ กันตั้งแต่ต่ำสุดถึงสูงสุด (รวมต่ำสุด/สูงสุดเสมอ)
      if(neededLevels <= 1){
        chosen = [groups[0]];
      } else {
        chosen = [];
        for(var k = 0; k < neededLevels; k++){
          var idx = Math.round(k * (groups.length - 1) / (neededLevels - 1));
          if(chosen.indexOf(groups[idx]) === -1) chosen.push(groups[idx]);
        }
        // กันกรณีจำนวนกลุ่มใกล้เคียง neededLevels จนมีบางตำแหน่งชนกัน (ปัดเศษซ้ำ) — เติมจากระดับที่ยังไม่ถูกเลือก
        if(chosen.length < neededLevels){
          groups.forEach(function(g){ if(chosen.length < neededLevels && chosen.indexOf(g) === -1) chosen.push(g); });
          chosen.sort(function(a, b){ return a.ref - b.ref; });
        }
      }
      warnings.push('มีค่าอ้างอิง ' + groups.length + ' ระดับ ใช้ ' + neededLevels + ' ระดับ (' + chosen.map(function(g){ return g.ref.toFixed(3); }).join(', ') + ') เพราะตารางมี ' + CAL_POINTS_TOTAL + ' จุด');
    }
    var pts = [{ raw:0, aw:0 }];
    chosen.forEach(function(g){ pts.push({ raw:g.raw, aw:Math.min(1, g.ref / g.gf) }); });

    // ตรวจคุณภาพข้อมูลรายกลุ่ม
    chosen.forEach(function(g){
      var tag = 'ระดับ ' + g.ref.toFixed(3);
      if(g.n < 2) warnings.push(tag + ': มีรอบเดียว (ควรวัดซ้ำอย่างน้อย 2-3 รอบเพื่อดูความซ้ำได้)');
      if(g.sd > 0.02) warnings.push(tag + ': ค่า raw ของรอบต่าง ๆ กระจาย ±' + g.sd.toFixed(3) + ' (เกิน 0.02) — วิธีวัดยังซ้ำไม่ได้ ควรรอถึงสมดุลจริงก่อน');
      if(g.maxDrift > 0.002) warnings.push(tag + ': ท้ายรอบค่ายังเปลี่ยนอยู่ ' + (g.maxDrift * 1000).toFixed(1) + ' ต่อพันต่อนาที — ยังไม่ถึงสมดุล');
      if(g.minDur < 5) warnings.push(tag + ': มีรอบที่วัดสั้นกว่า 5 นาที');
      if(g.tempSpread > 2) warnings.push(tag + ': อุณหภูมิระหว่างรอบต่างกัน ' + g.tempSpread.toFixed(1) + ' °C');
    });
    // ความชันแต่ละช่วง: เซนเซอร์จริงควรใกล้ 1 (SHT35 ±1.5 %RH) ถ้าต้องยืด/บีบมากแปลว่าปัญหาไม่ได้อยู่ที่เซนเซอร์
    var segs = [];
    for(var j = 1; j < pts.length; j++){
      var s = (pts[j].aw - pts[j - 1].aw) / (pts[j].raw - pts[j - 1].raw);
      segs.push({ from:pts[j - 1].raw, to:pts[j].raw, slope:s });
      if(j > 1 && (s > 1.3 || s < 0.75))
        warnings.push('ช่วง raw ' + pts[j - 1].raw.toFixed(3) + '→' + pts[j].raw.toFixed(3) + ': ต้องยืด/บีบสเกล ×' + s.toFixed(2) + ' ซึ่งเกินกว่าที่ตัวเซนเซอร์ควรคลาดมาก — สงสัยห้องวัดไม่ปิดผนึก/ยังไม่สมดุลก่อน (คาลิเบรตกลบปัญหานี้ได้เฉพาะเงื่อนไขเดิมเท่านั้น)');
    }
    var off = pts[1].aw - pts[1].raw;
    if(Math.abs(off) > 0.05) warnings.push('จุดต่ำสุดต้องเลื่อนค่า ' + (off >= 0 ? '+' : '') + off.toFixed(3) + ' (เกิน 0.05) — ผิดปกติสำหรับ SHT35');
    return { ok:true, groups:groups, chosen:chosen, points:pts, warnings:warnings, segments:segs };
  }
  // ==CALASSIST_END==

  function renderCalSuggestion(){
    var box = byId('calSuggest'); if(!box) return;
    var res = buildCalProposal(savedRuns);
    var h = '';
    function gTable(gs){
      return '<table style="margin-top:8px;"><thead><tr><th>ค่าอ้างอิง</th><th>จำนวนรอบ</th><th>raw เฉลี่ย ± SD</th><th>ตัวคูณอุณหภูมิ</th></tr></thead><tbody>' +
        gs.map(function(g){ return '<tr><td>' + g.ref.toFixed(3) + '</td><td>' + g.n + '</td><td>' + g.raw.toFixed(3) + ' ± ' + g.sd.toFixed(3) + '</td><td>×' + g.gf.toFixed(3) + '</td></tr>'; }).join('') + '</tbody></table>';
    }
    if(!res.ok){
      h = '<div class="hint" style="color:#ff8a8a;">' + escapeHtml(res.reason) + '</div>' +
          (res.conflicts ? '<ul class="hint">' + res.conflicts.map(function(c){ return '<li>' + escapeHtml(c) + '</li>'; }).join('') + '</ul>' : '') +
          (res.groups && res.groups.length ? gTable(res.groups) : '') +
          (res.warnings && res.warnings.length ? '<ul class="hint">' + res.warnings.map(function(c){ return '<li>' + escapeHtml(c) + '</li>'; }).join('') + '</ul>' : '');
      box.innerHTML = h; return;
    }
    h = '<div class="hint">ตารางที่เสนอ (จากรอบที่บันทึกไว้ ' + res.groups.reduce(function(s, g){ return s + g.n; }, 0) + ' รอบ)</div>' + gTable(res.chosen) +
        '<table style="margin-top:8px;"><thead><tr><th>จุด</th><th>raw</th><th>aw</th></tr></thead><tbody>' +
        res.points.map(function(p, i){ return '<tr><td>' + (i + 1) + '</td><td>' + p.raw.toFixed(4) + '</td><td>' + p.aw.toFixed(4) + '</td></tr>'; }).join('') + '</tbody></table>' +
        (res.warnings.length ? '<ul class="hint" style="color:#ffd27a;">' + res.warnings.map(function(c){ return '<li>' + escapeHtml(c) + '</li>'; }).join('') + '</ul>' : '<div class="hint" style="color:#7be0a5;">ไม่พบสัญญาณผิดปกติในข้อมูลชุดนี้</div>') +
        '<div class="btnrow" style="margin-top:8px;"><button class="primary" id="calApplyBtn">ใส่ลงช่องจุดคาลิเบรตด้านบน</button></div>' +
        '<div class="hint">กดแล้วยังไม่บันทึกลงเครื่อง — ตรวจตัวเลขแล้วกด "บันทึกจุดคาลิเบรต" อีกครั้ง หลังบันทึกควรวัดสารมาตรฐานอีกตัวที่ไม่ได้ใช้คาลิเบรตซ้ำเพื่อยืนยันความแม่นยำ</div>';
    box.innerHTML = h;
    var ab = byId('calApplyBtn');
    if(ab) ab.onclick = function(){ renderCalRows(res.points); byId('calHint').textContent = 'ใส่จุดที่เสนอแล้ว — ตรวจแล้วกด "บันทึกจุดคาลิเบรต"'; };
  }

  function saveCalPoints(){
    var raws = document.querySelectorAll('.calRaw'), aws = document.querySelectorAll('.calAw');
    var qs = [];
    for(var i=0;i<raws.length;i++){
      qs.push('p'+i+'raw=' + encodeURIComponent(raws[i].value));
      qs.push('p'+i+'aw=' + encodeURIComponent(aws[i].value));
    }
    fetch('/calset?' + qs.join('&')).then(function(r){
      return r.text().then(function(t){ return {ok:r.ok, t:t}; });
    }).then(function(res){
      byId('calHint').textContent = res.ok ? 'บันทึกจุดคาลิเบรตแล้ว ใช้งานทันที' : ('บันทึกไม่สำเร็จ: ' + res.t);
    }).catch(function(){ byId('calHint').textContent = 'บันทึกไม่สำเร็จ (เชื่อมต่อบอร์ดไม่ได้)'; });
  }

  function resetCalPoints(){
    if(!confirm('รีเซ็ตจุดคาลิเบรตกลับเป็นค่าโรงงาน?')) return;
    fetch('/calreset').then(function(){
      byId('calHint').textContent = 'รีเซ็ตเป็นค่าโรงงานแล้ว';
      return fetch('/calpoints').then(function(r){ return r.json(); }).then(function(d){ renderCalRows(d.points||[]); });
    }).catch(function(){ byId('calHint').textContent = 'รีเซ็ตไม่สำเร็จ (เชื่อมต่อบอร์ดไม่ได้)'; });
  }

  // v22: คาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก — ส่งค่า refAw ที่ผู้ใช้พิมพ์ไปให้เฟิร์มแวร์ อ่านค่าดิบสด
  // แล้วผสมเข้ากับตารางคาลิเบรตเดิม (ไม่ได้แทนที่ทั้งตาราง) จากนั้นโหลดตารางที่ปรับแล้วกลับมาวาดใหม่
  function quickCal(){
    var input = byId('quickCalRefAw');
    var v = parseFloat(input.value);
    if(isNaN(v) || v < 0 || v > 1){ byId('quickCalHint').textContent = 'กรอกค่า Aw อ้างอิง 0-1 ก่อน (เช่น 0.7530)'; return; }
    byId('quickCalHint').textContent = 'กำลังอ่านค่าดิบสดและปรับตาราง...';
    fetch('/calquick?refAw=' + encodeURIComponent(v)).then(function(r){
      return r.text().then(function(t){ return {ok:r.ok, t:t}; });
    }).then(function(res){
      if(!res.ok){ byId('quickCalHint').textContent = 'ปรับไม่สำเร็จ: ' + res.t; return; }
      var d;
      try { d = JSON.parse(res.t); } catch(e){ byId('quickCalHint').textContent = 'ปรับไม่สำเร็จ: ตอบกลับไม่ถูกรูปแบบ'; return; }
      byId('quickCalHint').textContent = 'ปรับแล้ว — raw สด ' + d.raw.toFixed(4) + ', ของเดิมทำนายได้ ' + d.predictedAwBefore.toFixed(4) +
        ' เทียบกับค่าอ้างอิง ' + d.refAw.toFixed(4) + ' (ส่วนต่าง ' + d.errAw.toFixed(4) + ') — บันทึกลงตารางแล้ว ยังผสมกับค่าคาลิเบรตเดิมอยู่';
      return fetch('/calpoints').then(function(r){ return r.json(); }).then(function(dd){ renderCalRows(dd.points||[]); });
    }).catch(function(){ byId('quickCalHint').textContent = 'ปรับไม่สำเร็จ (เชื่อมต่อบอร์ดไม่ได้ หรือกำลังวัด/คาลิเบตอัตโนมัติอยู่)'; });
  }

  // ============================================================
  //  ส่งออกไฟล์ (CSV / PNG) — แนบหัวไฟล์เมทาดาทาให้ตรวจสอบย้อนกลับได้
  // ============================================================
  function buildMetaHeader(title){
    var lines = [];
    lines.push('# ' + title);
    lines.push('# Exported at (local browser time),' + new Date().toISOString());
    if(lastData){
      lines.push('# Value mode,' + valueMode);
      lines.push('# Operator,' + (lastData.operator || ''));
    }
    lines.push('');
    return lines.join('\n') + '\n';
  }

  // ============================================================
  //  v12: กราฟส่งออกแบบเอกสารทางการ (พื้นขาว) — ใช้แนบอ้างอิงคู่กับใบรายงานผล (COA)
  // ============================================================
  // วาดลงแคนวาสแยกต่างหาก (ไม่แตะกราฟสดบนแดชบอร์ดที่เป็นธีมมืด) พื้นขาว ตัวอักษรดำ เส้นกริดสีเทาอ่อน
  // มีหัวกระดาษ/เลขที่กราฟ (Graph No.) / ข้อมูลตัวอย่าง / เงื่อนไขการทดสอบ / ตารางสรุปค่า / คำเตือนท้ายกระดาษ
  // ฟังก์ชัน renderFormalChart() เป็นฟังก์ชันล้วน (รับ makeCanvas เข้ามา) จึงทดสอบนอกเบราว์เซอร์ได้
  // ==FORMAL_CHART_BEGIN==
  var FORMAL_COLORS = ['#0b3d91','#b3261e','#1b7f3b','#6a1b9a','#e65100','#00695c','#4e342e','#37474f'];
  var FORMAL_DASH = [[],[12,6],[3,5],[14,5,3,5],[8,4],[2,4],[16,6],[5,3]];
  var FORMAL_FONT = '"Segoe UI", Tahoma, "Noto Sans Thai", Thonburi, Arial, Helvetica, sans-serif';

  function niceStep(range, ticks){
    var raw = range / ticks;
    if(!(raw > 0)) return 1;
    var mag = Math.pow(10, Math.floor(Math.log10(raw))), n = raw / mag;
    return (n <= 1 ? 1 : n <= 2 ? 2 : n <= 5 ? 5 : 10) * mag;
  }

  // field: 'aw' (คาลิเบรต, ค่าเริ่มต้น) หรือ 'raw' (ดิบก่อนคาลิเบรต) — ใช้ตัดสินว่าตารางสรุป/จุดสุดท้ายอ้างอิงเส้นไหน
  function summarizeSeries(s, field){
    field = field || 'aw';
    var pts = s.points, n = pts.length;
    var out = { n:n, durMin:0, last:null, min:null, max:null, tMean:null, awMean:null, errPct:null };
    if(!n) return out;
    var last = pts[n-1];
    var lastV = (typeof last[field] === 'number' && !isNaN(last[field])) ? last[field] : last.aw;
    out.durMin = Math.max(0, last.t - pts[0].t);
    out.last = lastV; out.min = lastV; out.max = lastV;
    var ts = 0, tn = 0, aws = 0, awn = 0;
    pts.forEach(function(p){
      var v = (typeof p[field] === 'number' && !isNaN(p[field])) ? p[field] : null;
      if(v === null) return;
      if(v < out.min) out.min = v;
      if(v > out.max) out.max = v;
      aws += v; awn++;
      if(typeof p.temp === 'number' && p.temp > -50){ ts += p.temp; tn++; }
    });
    if(tn) out.tMean = ts / tn;
    if(awn) out.awMean = aws / awn;
    if(typeof s.refAw === 'number' && !isNaN(s.refAw) && s.refAw !== 0) out.errPct = (lastV - s.refAw) / s.refAw * 100;
    return out;
  }

  function renderFormalChart(opt, makeCanvas){
    var series = (opt.series || []).filter(function(s){ return s.points && s.points.length; });
    var meta = opt.meta || {};
    var single = (series.length === 1);
    var nRows = Math.max(1, series.length);
    var W = 1600;
    var H = 1130 + Math.max(0, nRows - 3) * 30;
    var c = makeCanvas(W, H), g = c.getContext('2d');
    var INK = '#111111', MUTED = '#555555', GRID = '#dddddd', GRID_MID = '#bdbdbd', AXIS = '#222222';
    // v-formal-mode: 'aw' = ค่าคาลิเบรต (ทางการ, ค่าเริ่มต้น) / 'raw' = ค่าดิบก่อนคาลิเบรต / 'both' = ซ้อนสองเส้น
    var plotField = (meta.plotField === 'raw' || meta.plotField === 'both') ? meta.plotField : 'aw';
    var showRawOverlay = (plotField === 'both');
    var primaryField = (plotField === 'raw') ? 'raw' : 'aw'; // เส้น/ตารางหลักอ้างอิงฟิลด์นี้ ('both' ยังถือ aw เป็นค่าทางการหลัก)
    var roomField = (plotField === 'raw') ? 'roomRaw' : 'roomAw';
    var valueLabel = (plotField === 'raw') ? 'Raw sensor value (RH/100, uncalibrated) — zoomed axis' : 'Water activity, aw';
    var valDec = (plotField === 'raw') ? 4 : 3;   // ค่าดิบ: ความละเอียด 0.0001 (= 0.01 %RH)
    var valueLabelShort = (plotField === 'raw') ? 'raw' : 'aw';

    function setFont(weight, px){ g.font = weight + ' ' + px + 'px ' + FORMAL_FONT; }
    function text(s, x, y, color, weight, px, align){
      setFont(weight || '400', px || 18);
      g.fillStyle = color || INK; g.textAlign = align || 'left'; g.textBaseline = 'alphabetic';
      g.fillText(String(s), x, y);
    }
    function fit(s, maxW, weight, px){
      setFont(weight || '400', px || 18);
      s = String(s);
      if(g.measureText(s).width <= maxW) return s;
      while(s.length > 1 && g.measureText(s + '…').width > maxW) s = s.slice(0, -1);
      return s + '…';
    }
    function hline(x1, x2, y, color, w){ g.strokeStyle = color; g.lineWidth = w; g.beginPath(); g.moveTo(x1, y); g.lineTo(x2, y); g.stroke(); }
    function vline(x, y1, y2, color, w){ g.strokeStyle = color; g.lineWidth = w; g.beginPath(); g.moveTo(x, y1); g.lineTo(x, y2); g.stroke(); }
    function num(v, d){ return (typeof v === 'number' && !isNaN(v)) ? v.toFixed(d) : '–'; }

    // ---- พื้นหลังขาว + กรอบ ----
    g.fillStyle = '#ffffff'; g.fillRect(0, 0, W, H);
    g.strokeStyle = '#8a8a8a'; g.lineWidth = 2; g.strokeRect(20, 20, W - 40, H - 40);

    // ---- หัวกระดาษ ----
    var titleTxt = (plotField === 'raw') ? 'Water Activity — RAW Sensor Reading (Uncalibrated) — Trend Graph'
      : (plotField === 'both') ? 'Water Activity (aw) — Calibrated vs Raw — Trend Graph'
      : 'Water Activity (aw) Measurement — Trend Graph';
    var subtitleTxt = (plotField === 'raw') ? 'กราฟแนวโน้มค่าดิบจากเซนเซอร์ (ก่อนคาลิเบรต) — ใช้เทียบเคียง/อ้างอิงเท่านั้น ไม่ใช่ค่า aw ทางการ'
      : (plotField === 'both') ? 'กราฟแนวโน้มค่าคาลิเบรต (เส้นทึบ, ค่าทางการ) ซ้อนกับค่าดิบก่อนคาลิเบรต (เส้นประ, อ้างอิง)'
      : 'กราฟแนวโน้มการวัดค่า Water Activity (aw) และอุณหภูมิ';
    text(titleTxt, 60, 84, INK, '700', 34);
    text(subtitleTxt, 60, 116, MUTED, '400', 20);
    text('Instrument: ' + (meta.deviceName || 'AW Meter'), 60, 144, MUTED, '400', 18);
    text('Graph No.', W - 60, 62, MUTED, '400', 15, 'right');
    text(meta.graphNo || '–', W - 60, 94, INK, '700', 28, 'right');
    text('Issued: ' + (meta.issuedAt || '–'), W - 60, 122, MUTED, '400', 16, 'right');
    text('Related test report: see "Graph Ref." in the Test Report', W - 60, 144, MUTED, '400', 14, 'right');
    hline(60, W - 60, 160, INK, 2.5);

    // ---- ข้อมูลตัวอย่าง / การวัด ----
    var sums = series.map(function(s){ return summarizeSeries(s, primaryField); });
    var totalPts = 0, maxDur = 0;
    sums.forEach(function(x){ totalPts += x.n; if(x.durMin > maxDur) maxDur = x.durMin; });
    var leftRows = [
      ['Sample name', meta.sampleName],
      ['Sample ID / Lot No.', meta.sampleId],
      ['Customer / Organization', meta.customer],
      ['Dataset', single ? meta.datasetName : (series.length + ' datasets (comparison)')],
      ['Operator / Analyst', meta.operator]
    ];
    var rightRows = [
      ['Recorded (local time)', meta.recordedAt],
      ['Measurement duration', num(maxDur, 1) + ' min'],
      ['Data points', String(totalPts)],
      ['Value mode', (meta.valueMode || '–') + ' — graph shows ' + (plotField === 'raw' ? 'RAW' : plotField === 'both' ? 'CAL + RAW' : 'CAL')],
      ['Firmware', meta.fwVersion]
    ];
    var colW = (W - 120) / 2 - 20, labelW = 235, rowY0 = 196, rowH = 30;
    [[leftRows, 60], [rightRows, 60 + colW + 40]].forEach(function(col){
      col[0].forEach(function(r, i){
        var y = rowY0 + i * rowH;
        text(r[0], col[1], y, MUTED, '400', 16);
        text(fit((r[1] === undefined || r[1] === null || r[1] === '') ? '–' : r[1], colW - labelW - 8, '600', 17), col[1] + labelW, y, INK, '600', 17);
      });
    });

    // เงื่อนไขการทดสอบ (ตัดบรรทัดอัตโนมัติ สูงสุด 2 บรรทัด)
    var condY = rowY0 + leftRows.length * rowH + 14;
    text('Test conditions', 60, condY, MUTED, '400', 16);
    var parts = (meta.conditions || []).slice(), lines = [], cur = '', maxCondW = W - 120 - labelW;
    setFont('400', 16);
    parts.forEach(function(p){
      var trial = cur ? (cur + '   |   ' + p) : p;
      if(g.measureText(trial).width > maxCondW && cur){ lines.push(cur); cur = p; } else { cur = trial; }
    });
    if(cur) lines.push(cur);
    if(!lines.length) lines.push('–');
    lines.slice(0, 2).forEach(function(l, i){ text(fit(l, maxCondW, '400', 16), 60 + labelW, condY + i * 24, INK, '400', 16); });

    // ---- โครงพื้นที่กราฟ (คำนวณจากจำนวนแถวตาราง/แถวคำอธิบาย) ----
    var hasRoomBaseline = single && series[0].points.some(function(p){ return typeof p[roomField] === 'number' && !isNaN(p[roomField]); });
    var legendItems = series.length + (single ? 1 : 0) + (showRawOverlay && single ? 1 : 0) + (hasRoomBaseline ? 1 : 0);
    var legendCols = Math.min(4, Math.max(1, legendItems));
    var legendRows = Math.ceil(legendItems / legendCols);
    var tableH = (nRows + 1) * 30 + 8;
    var tableTop = H - 90 - tableH;
    var plotL = 140, plotR = W - 140, plotT = condY + 50;
    var plotB = tableTop - 112 - (legendRows - 1) * 26;
    var plotW = plotR - plotL, plotH = plotB - plotT;

    var tMax = 1;
    series.forEach(function(s){ var t0 = s.points[0].t, tl = s.points[s.points.length - 1].t - t0; if(tl > tMax) tMax = tl; });
    var xStep = niceStep(tMax, 8);
    var xMax = Math.max(xStep, Math.ceil(tMax / xStep - 1e-9) * xStep);
    var TMIN = 0, TMAX = 60;
    function xPix(t){ return plotL + (t / xMax) * plotW; }
    // ---- แกน Y: โหมด raw ซูมตามช่วงข้อมูลจริง (เดิมตายตัว 0-1 ทำให้เส้นที่ต่างกัน 0.02-0.1 อัดกันอยู่แถบเดียว) ----
    var yLo = 0, yHi = 1, yStepV = 0.1, yDec = 1;
    if(plotField === 'raw'){
      var zmin = Infinity, zmax = -Infinity;
      series.forEach(function(s){ s.points.forEach(function(p){
        if(typeof p.raw === 'number' && !isNaN(p.raw)){ if(p.raw < zmin) zmin = p.raw; if(p.raw > zmax) zmax = p.raw; }
        if(typeof p.roomRaw === 'number' && !isNaN(p.roomRaw)){ if(p.roomRaw < zmin) zmin = p.roomRaw; if(p.roomRaw > zmax) zmax = p.roomRaw; }
      }); });
      if(zmin <= zmax){
        var zspan = Math.max(zmax - zmin, 0.01), zpad = zspan * 0.12;
        yStepV = niceStep(zspan + 2 * zpad, 8);
        yLo = Math.max(0, Math.floor((zmin - zpad) / yStepV + 1e-9) * yStepV);
        yHi = Math.min(1, Math.ceil((zmax + zpad) / yStepV - 1e-9) * yStepV);
        if(!(yHi > yLo)){ yLo = 0; yHi = 1; yStepV = 0.1; }
        yDec = Math.max(1, Math.min(5, Math.ceil(-Math.log10(yStepV) - 1e-9)));
      }
    }
    function yAw(v){ return plotB - ((Math.max(yLo, Math.min(yHi, v)) - yLo) / (yHi - yLo)) * plotH; }
    function yTemp(v){ return plotB - ((Math.max(TMIN, Math.min(TMAX, v)) - TMIN) / (TMAX - TMIN)) * plotH; }

    // กริด + สเกล
    var i, k;
    var nyT = Math.max(1, Math.round((yHi - yLo) / yStepV));
    for(i = 0; i <= nyT; i++){
      var gv = yLo + i * yStepV, yy = yAw(gv);
      var isMid = (plotField !== 'raw' && i === 5);
      hline(plotL, plotR, yy, isMid ? GRID_MID : GRID, isMid ? 1.4 : 1);
      hline(plotL - 7, plotL, yy, AXIS, 1.6);
      text(gv.toFixed(yDec), plotL - 14, yy + 5, INK, '400', 16, 'right');
    }
    var nx = Math.round(xMax / xStep);
    for(k = 0; k <= nx; k++){
      var xv = k * xStep, xx = xPix(xv);
      vline(xx, plotT, plotB, GRID, 1);
      vline(xx, plotB, plotB + 7, AXIS, 1.6);
      text(xv.toFixed(xStep < 1 ? 1 : 0), xx, plotB + 27, INK, '400', 16, 'center');
    }
    if(single){
      for(i = 0; i <= 6; i++){
        var ty = yTemp(TMIN + i * 10);
        hline(plotR, plotR + 7, ty, AXIS, 1.6);
        text(String(TMIN + i * 10), plotR + 14, ty + 5, MUTED, '400', 16, 'left');
      }
    }

    // เส้นข้อมูล (clip อยู่ในกรอบกราฟ)
    g.save();
    g.beginPath(); g.rect(plotL, plotT, plotW, plotH); g.clip();
    if(hasRoomBaseline){
      var roomPtFormal = series[0].points.find(function(p){ return typeof p[roomField] === 'number' && !isNaN(p[roomField]); });
      var roomLineVal = roomPtFormal[roomField];
      g.setLineDash([10, 6]); g.strokeStyle = '#8b5cf6'; g.lineWidth = 2;
      g.beginPath(); g.moveTo(plotL, yAw(roomLineVal)); g.lineTo(plotR, yAw(roomLineVal)); g.stroke(); g.setLineDash([]);
    }
    series.forEach(function(s, si){
      var t0 = s.points[0].t;
      var col = FORMAL_COLORS[si % FORMAL_COLORS.length];
      if(single){
        g.setLineDash([7, 5]); g.strokeStyle = '#777777'; g.lineWidth = 1.8; g.beginPath();
        var started = false;
        s.points.forEach(function(p){
          if(!(typeof p.temp === 'number') || p.temp <= -50){ started = false; return; }
          var x = xPix(p.t - t0), y = yTemp(p.temp);
          if(!started){ g.moveTo(x, y); started = true; } else g.lineTo(x, y);
        });
        g.stroke();
      }
      g.setLineDash(FORMAL_DASH[si % FORMAL_DASH.length]);
      g.strokeStyle = col; g.lineWidth = 2.8; g.lineJoin = 'round'; g.beginPath();
      s.points.forEach(function(p, idx){
        var pv = (typeof p[primaryField] === 'number' && !isNaN(p[primaryField])) ? p[primaryField] : p.aw;
        var x = xPix(p.t - t0), y = yAw(pv);
        if(idx === 0) g.moveTo(x, y); else g.lineTo(x, y);
      });
      g.stroke();
      g.setLineDash([]);

      // v-formal-mode 'both': ซ้อนเส้นค่าดิบ (ก่อนคาลิเบรต) เป็นเส้นประสีเหลืองอำพันทับเส้นคาลิเบรตหลัก — เฉพาะกราฟชุดเดียว
      // (หลายชุด/หลายรอบซ้อนกันจะรกเกินไป จึงจำกัดไว้เฉพาะกรณี single เหมือนกับกราฟสดบนแดชบอร์ด)
      if(showRawOverlay && single){
        g.setLineDash([3, 4]); g.strokeStyle = '#c77700'; g.lineWidth = 2; g.beginPath();
        var startedRaw = false;
        s.points.forEach(function(p){
          if(typeof p.raw !== 'number' || isNaN(p.raw)){ startedRaw = false; return; }
          var x = xPix(p.t - t0), y = yAw(p.raw);
          if(!startedRaw){ g.moveTo(x, y); startedRaw = true; } else g.lineTo(x, y);
        });
        g.stroke();
        g.setLineDash([]);
      }
    });
    g.restore();

    // จุดสุดท้าย + ป้ายค่า (เฉพาะกราฟชุดเดียว)
    if(single){
      var sp = series[0].points, lp = sp[sp.length - 1];
      var lpVal = (typeof lp[primaryField] === 'number' && !isNaN(lp[primaryField])) ? lp[primaryField] : lp.aw;
      var lx = xPix(lp.t - sp[0].t), ly = yAw(lpVal);
      g.fillStyle = FORMAL_COLORS[0]; g.beginPath(); g.arc(lx, ly, 6, 0, Math.PI * 2); g.fill();
      g.strokeStyle = '#ffffff'; g.lineWidth = 2; g.beginPath(); g.arc(lx, ly, 6, 0, Math.PI * 2); g.stroke();
      var lbl = 'Final ' + valueLabelShort + ' = ' + num(lpVal, valDec);
      setFont('700', 18);
      var lw = g.measureText(lbl).width + 20, bx = Math.min(lx - lw - 8, plotR - lw - 4), by = (ly - 44 > plotT) ? ly - 44 : ly + 16;
      if(bx < plotL + 4) bx = plotL + 4;
      g.fillStyle = '#ffffff'; g.fillRect(bx, by, lw, 28);
      g.strokeStyle = FORMAL_COLORS[0]; g.lineWidth = 1.5; g.strokeRect(bx, by, lw, 28);
      text(lbl, bx + 10, by + 20, INK, '700', 18);
    }

    // กรอบแกน
    g.strokeStyle = AXIS; g.lineWidth = 2; g.setLineDash([]); g.strokeRect(plotL, plotT, plotW, plotH);

    // ชื่อแกน
    text('Elapsed time (min)', plotL + plotW / 2, plotB + 58, INK, '600', 18, 'center');
    g.save(); g.translate(64, plotT + plotH / 2); g.rotate(-Math.PI / 2);
    text(valueLabel, 0, 0, INK, '600', 18, 'center'); g.restore();
    if(single){
      g.save(); g.translate(W - 64, plotT + plotH / 2); g.rotate(-Math.PI / 2);
      text('Temperature (°C)', 0, 0, MUTED, '600', 18, 'center'); g.restore();
    }

    // คำอธิบายเส้น (legend)
    var legY = plotB + 96, legColW = plotW / legendCols;
    function legendItem(idx, label, color, dash, lw){
      var cx = plotL + (idx % legendCols) * legColW, cy = legY + Math.floor(idx / legendCols) * 26;
      g.setLineDash(dash); g.strokeStyle = color; g.lineWidth = lw;
      g.beginPath(); g.moveTo(cx, cy - 6); g.lineTo(cx + 46, cy - 6); g.stroke(); g.setLineDash([]);
      text(fit(label, legColW - 70, '400', 16), cx + 56, cy, INK, '400', 16);
    }
    series.forEach(function(s, si){
      legendItem(si, single ? (valueLabel + ' (left axis)') : s.name, FORMAL_COLORS[si % FORMAL_COLORS.length], FORMAL_DASH[si % FORMAL_DASH.length], 2.8);
    });
    var legIdx = series.length;
    if(single){ legendItem(legIdx, 'Temperature, °C (right axis)', '#777777', [7, 5], 1.8); legIdx++; }
    if(showRawOverlay && single){ legendItem(legIdx, 'Raw sensor value, RH/100 (uncalibrated, left axis)', '#c77700', [3, 4], 2); legIdx++; }
    if(hasRoomBaseline){ legendItem(legIdx, (roomField === 'roomRaw' ? 'Room baseline, RH/100' : 'Room baseline, aw'), '#8b5cf6', [10, 6], 2); legIdx++; }

    // ตารางสรุป
    var tL = 60, tW = W - 120;
    // v-formal-mode: หัวคอลัมน์ตัวเลขอ้างอิงฟิลด์ที่กำลังพล็อตจริง (aw คาลิเบรตปกติ, หรือ raw เมื่อเลือกโหมด RAW) —
    // โหมด 'both' ตารางยังอ้างอิงค่า aw คาลิเบรต (ค่าทางการ) เป็นหลัก เส้นดิบเป็นภาพซ้อนอ้างอิงบนกราฟเท่านั้น
    var colValLbl = (valueLabelShort === 'raw') ? 'raw' : 'aw';
    var cols = [
      ['Dataset', 380, 'left'], ['Points', 90, 'right'], ['Duration (min)', 140, 'right'], ['Final ' + colValLbl, 130, 'right'],
      ['Min ' + colValLbl, 115, 'right'], ['Max ' + colValLbl, 115, 'right'], ['Mean ' + colValLbl, 115, 'right'], ['Mean temp (°C)', 140, 'right'], ['Ref. aw', 115, 'right'], ['Error (%)', 140, 'right']
    ];
    var tRowH = 30, y0 = tableTop;
    g.fillStyle = '#efefef'; g.fillRect(tL, y0, tW, tRowH);
    g.strokeStyle = '#9a9a9a'; g.lineWidth = 1;
    g.strokeRect(tL, y0, tW, tRowH * (nRows + 1));
    var cx0 = tL;
    cols.forEach(function(col, ci){
      var cw = col[1];
      var tx = (col[2] === 'right') ? cx0 + cw - 12 : cx0 + 12;
      text(fit(col[0], cw - 22, '700', 15), tx, y0 + 21, INK, '700', 15, col[2]);
      if(ci > 0) vline(cx0, y0, y0 + tRowH * (nRows + 1), '#bbbbbb', 1);
      cx0 += cw;
    });
    series.forEach(function(s, si){
      var st = sums[si], ry = y0 + tRowH * (si + 1);
      hline(tL, tL + tW, ry, '#bbbbbb', 1);
      var cells = [
        (single ? (meta.datasetName || s.name) : s.name), String(st.n), num(st.durMin, 1), num(st.last, valDec),
        num(st.min, valDec), num(st.max, valDec), num(st.awMean, valDec), num(st.tMean, 1), num(s.refAw, 3), (st.errPct === null ? '–' : (st.errPct >= 0 ? '+' : '') + st.errPct.toFixed(2))
      ];
      var cx = tL;
      cols.forEach(function(col, ci){
        var cw = col[1], tx = (col[2] === 'right') ? cx + cw - 12 : cx + 12;
        text(fit(cells[ci], cw - 22, ci === 3 ? '700' : '400', 16), tx, ry + 21, INK, ci === 3 ? '700' : '400', 16, col[2]);
        cx += cw;
      });
    });

    // ท้ายกระดาษ
    hline(60, W - 60, H - 78, '#999999', 1);
    text('Generated by AW Meter Dashboard  •  ' + (meta.fwVersion || '') + '  •  ' + (meta.graphNo || '') + '  •  Exported ' + (meta.issuedAt || ''), 60, H - 54, MUTED, '400', 14);
    text('This graph is a traceability reference only and is not a certificate accredited under ISO/IEC 17025. เอกสารนี้ใช้เพื่อการตรวจสอบย้อนกลับเท่านั้น ไม่ใช่ใบรับรองที่ได้รับการรับรองมาตรฐาน', 60, H - 32, MUTED, '400', 14);
    return c;
  }
  // ==FORMAL_CHART_END==

  // ============================================================
  //  v12: เลขที่อ้างอิงข้อมูล/กราฟ + ตัวช่วยส่งออกกราฟพื้นขาว
  // ============================================================
  // Data ID ผูกกับชุดข้อมูลหนึ่งชุด (รอบที่บันทึกไว้ = คงที่ตลอดกาลจากเลข id ของรอบนั้น, กราฟสด = คงที่ตลอดรอบบันทึกปัจจุบัน)
  // เลขที่กราฟ = "AWG-" + Data ID  และใบรายงาน (COA) ระบุเลขนี้ไว้ในช่อง "กราฟอ้างอิง (Graph Ref.)" ให้ตรงกัน
  var liveDataId = null;
  var liveStartedIso = null;
  function pad2(n){ return (n < 10 ? '0' : '') + n; }
  function ymd(d){ return d.getFullYear() + pad2(d.getMonth() + 1) + pad2(d.getDate()); }
  function fmtLocal(d){
    return d.getFullYear() + '-' + pad2(d.getMonth() + 1) + '-' + pad2(d.getDate()) + ' ' +
           pad2(d.getHours()) + ':' + pad2(d.getMinutes()) + ':' + pad2(d.getSeconds());
  }
  function idSuffix(n){ return ('00000' + Math.abs(Math.floor(n)).toString(36).toUpperCase()).slice(-5); }
  function dataIdForRun(run){
    var ts = parseInt(String(run.id).replace(/\D/g, ''), 10);
    if(isNaN(ts)) ts = Date.parse(run.savedAt) || Date.now();
    return ymd(new Date(ts)) + '-' + idSuffix(ts);
  }
  function dataIdForLive(){
    if(!liveDataId){ var now = Date.now(); liveDataId = ymd(new Date(now)) + '-' + idSuffix(now); }
    return liveDataId;
  }

  function getReportForm(){
    var d = deviceInfoCache || {};
    return {
      sampleName: byId('rptSampleName').value.trim(),
      sampleId: byId('rptSampleId').value.trim(),
      customer: byId('rptCustomer').value.trim(),
      analyst: byId('rptAnalyst').value.trim() || (d.operator || (lastData && lastData.operator) || ''),
      approver: byId('rptApprover').value.trim()
    };
  }

  // ชุดข้อมูลหนึ่งชุดที่เลือกจากดรอปดาวน์ "ข้อมูลที่จะใช้ออกรายงาน" (ใช้ร่วมกันทั้งใบรายงานและกราฟอ้างอิง)
  function resolveSource(sourceId){
    if(sourceId === 'live'){
      return { name:'ค่าปัจจุบัน (Live)', points:dataPoints, refAw:null,
               savedAt: liveStartedIso || new Date().toISOString(), dataId: dataIdForLive() };
    }
    var run = savedRuns.find(function(r){ return r.id === sourceId; });
    if(!run) return null;
    return { name:run.name, points:run.points, refAw:run.refAw, savedAt:run.savedAt, dataId:dataIdForRun(run) };
  }

  // สิ่งที่เห็นอยู่บนกราฟหน้าแดชบอร์ดตอนนี้ (Live ถ้าติ๊กไว้ + รอบที่บันทึกไว้ที่ติ๊กแสดง)
  function collectVisibleSources(){
    var out = [];
    if(byId('liveToggle').checked && dataPoints.length) out.push(resolveSource('live'));
    savedRuns.filter(function(r){ return r.visible; }).forEach(function(r){ out.push(resolveSource(r.id)); });
    return out.filter(function(s){ return s && s.points.length; });
  }

  function conditionsList(){
    var d = deviceInfoCache || {}, out = [];
    if(typeof d.targetTempC === 'number') out.push('Target temperature ' + d.targetTempC.toFixed(1) + ' °C');
    if(typeof d.shtPreHeatSec === 'number')
      out.push('SHT heater: pre ' + d.shtPreHeatSec + ' s heat + ' + d.shtPreCoolSec + ' s recovery, post ' + d.shtPostHeatSec + ' s heat');
    if(typeof d.stabToleranceAw === 'number')
      // v-cal-fix: เดิมอ้าง stabGraphWindowPts/Sec (หน้าต่างกราฟบนจอ ~15-22 วิ) ซึ่งไม่ใช่เกณฑ์ตัดสิน "นิ่ง" จริงอีกต่อไป
      // (ดูคำอธิบายที่ updateStabilityWindow() ในไฟล์ .ino) — ตอนนี้อ้าง stabWindowSec ซึ่งคือหน้าต่างที่ใช้ตัดสินจริง
      out.push('Stable when aw range ≤ ' + d.stabToleranceAw.toFixed(4) + ' over the last ' +
               (typeof d.stabWindowSec === 'number' ? d.stabWindowSec : '–') + ' s' +
               (typeof d.stabSlopeMaxPerMin === 'number' ? ' and drift ≤ ' + d.stabSlopeMaxPerMin.toFixed(4) + ' aw/min (2-min slope)' : '') +
               (d.minMeasureDurationSec ? ', min. ' + (d.minMeasureDurationSec >= 120 ? Math.round(d.minMeasureDurationSec / 60) + ' min' : d.minMeasureDurationSec + ' s') + ' measurement' : ''));
    return out;
  }

  function downloadFormalGraph(sources){
    var withPts = (sources || []).filter(function(s){ return s && s.points && s.points.length; });
    if(!withPts.length){ alert('ยังไม่มีข้อมูลในกราฟให้ส่งออก — กด "เริ่มบันทึกกราฟ" หรือเลือกชุดข้อมูลที่บันทึกไว้ก่อน'); return; }
    var single = (withPts.length === 1);
    var d = deviceInfoCache || {}, f = getReportForm();
    var graphNo = single ? ('AWG-' + withPts[0].dataId) : ('AWG-CMP-' + ymd(new Date()) + '-' + idSuffix(Date.now()));
    var recorded = single ? withPts[0].savedAt : new Date().toISOString();
    // v-formal-mode: ค่าที่จะพล็อตในกราฟทางการนี้ — 'aw' (คาลิเบรต, ค่าทางการ) / 'raw' (ดิบก่อนคาลิเบรต) / 'both' (ซ้อนสองเส้น)
    // อ่านจากดรอปดาวน์ #formalValueModeSel เดียวกันทั้งปุ่ม "ดาวน์โหลดกราฟ PNG" และปุ่ม "ดาวน์โหลดกราฟอ้างอิง"
    var formalModeEl = byId('formalValueModeSel');
    var plotField = formalModeEl ? formalModeEl.value : 'aw';
    var meta = {
      graphNo: graphNo,
      deviceName: d.deviceName || 'AW Meter',
      fwVersion: d.fwVersion || '',
      valueMode: d.valueMode || valueMode,
      plotField: plotField,
      sampleName: f.sampleName, sampleId: f.sampleId, customer: f.customer,
      datasetName: single ? withPts[0].name : '',
      operator: f.analyst,
      recordedAt: (function(){ try{ return fmtLocal(new Date(recorded)); }catch(e){ return '–'; } })(),
      issuedAt: fmtLocal(new Date()),
      conditions: conditionsList()
    };
    var cv = renderFormalChart({
      series: withPts.map(function(s){ return { name:s.name, points:s.points, refAw:s.refAw }; }),
      meta: meta
    }, function(w, h){ var c = document.createElement('canvas'); c.width = w; c.height = h; return c; });
    var a = document.createElement('a');
    a.href = cv.toDataURL('image/png');
    a.download = graphNo + '.png';
    a.click();
  }

  // ============================================================
  //  v-trend-offset: ดาวน์โหลดกราฟ Offset / เส้นโค้งคาลิเบรตสด / ตารางสรุปโซน แบบเอกสารทางการ (พื้นขาว)
  //  ใช้จานสี/ฟอนต์/หัวกระดาษ-ท้ายกระดาษชุดเดียวกับ renderFormalChart() ด้านบน เพื่อให้เอกสารชุดเดียวกันดูสม่ำเสมอ
  // ============================================================
  function formalMeta(prefix){
    var d = deviceInfoCache || {};
    return {
      graphNo: 'AWG-' + prefix + '-' + ymd(new Date()) + '-' + idSuffix(Date.now()),
      deviceName: d.deviceName || 'AW Meter', fwVersion: d.fwVersion || '', issuedAt: fmtLocal(new Date())
    };
  }
  function drawFormalFrame(g, W, H, title, subtitle, meta){
    g.fillStyle = '#ffffff'; g.fillRect(0, 0, W, H);
    g.strokeStyle = '#8a8a8a'; g.lineWidth = 2; g.strokeRect(20, 20, W - 40, H - 40);
    function t(s, x, y, color, weight, px, align){
      g.font = (weight || '400') + ' ' + (px || 18) + 'px ' + FORMAL_FONT;
      g.fillStyle = color || '#111111'; g.textAlign = align || 'left'; g.fillText(String(s), x, y);
    }
    t(title, 60, 80, '#111111', '700', 30);
    var maxW = W - 220; g.font = '400 16px ' + FORMAL_FONT;
    var words = String(subtitle).split(' '), line = '', lines = [];
    words.forEach(function(w){ var trial = line ? (line + ' ' + w) : w; if(g.measureText(trial).width > maxW && line){ lines.push(line); line = w; } else line = trial; });
    if(line) lines.push(line);
    lines.slice(0, 2).forEach(function(l, i){ t(l, 60, 110 + i * 22, '#555555', '400', 16); });
    t('Instrument: ' + (meta.deviceName || 'AW Meter') + '   Firmware: ' + (meta.fwVersion || '–'), 60, 156, '#555555', '400', 15);
    t('Graph No.', W - 60, 62, '#555555', '400', 15, 'right');
    t(meta.graphNo || '–', W - 60, 94, '#111111', '700', 26, 'right');
    t('Issued: ' + (meta.issuedAt || '–'), W - 60, 120, '#555555', '400', 15, 'right');
    g.strokeStyle = '#111111'; g.lineWidth = 2.5; g.beginPath(); g.moveTo(60, 168); g.lineTo(W - 60, 168); g.stroke();
  }
  function drawFormalFooter(g, W, H, meta){
    g.strokeStyle = '#999999'; g.lineWidth = 1; g.beginPath(); g.moveTo(60, H - 56); g.lineTo(W - 60, H - 56); g.stroke();
    g.font = '400 13px ' + FORMAL_FONT; g.fillStyle = '#555555'; g.textAlign = 'left';
    g.fillText('Generated by AW Meter Dashboard  •  ' + (meta.fwVersion || '') + '  •  ' + (meta.graphNo || '') + '  •  Exported ' + (meta.issuedAt || ''), 60, H - 34);
    g.fillText('Traceability reference only — not an ISO/IEC 17025 accredited certificate. เอกสารนี้ใช้เพื่อการตรวจสอบย้อนกลับเท่านั้น ไม่ใช่ใบรับรองมาตรฐาน', 60, H - 14);
  }
  function downloadOffsetFormalPng(){
    if(!offLog.length){ alert('ยังไม่มีข้อมูลกราฟ Offset ให้ส่งออก — เริ่มวัดที่ตัวเครื่อง หรือรอให้ระบบเก็บข้อมูลสักครู่'); return; }
    var chartCv = document.createElement('canvas'); chartCv.width = 1400; chartCv.height = 680;
    drawOffsetChart(chartCv, 'formal');
    var meta = formalMeta('OFF');
    var W = chartCv.width + 120, H = chartCv.height + 280;
    var cv = document.createElement('canvas'); cv.width = W; cv.height = H; var g = cv.getContext('2d');
    drawFormalFrame(g, W, H, 'Automatic Offset Adjustment — Trend Graph',
      'เส้นน้ำเงิน = aw หลังปรับ, เส้นเทา = aw จากตารางก่อนปรับ, เส้นประส้ม = aw เป้าหมายของโซน ณ ขณะนั้น, เส้นตั้งแดง = offset ที่ปรับ, เส้นตั้งเขียว = error ที่เหลือ, กราฟล่าง = offset ตามเวลา', meta);
    g.drawImage(chartCv, 60, 190);
    drawFormalFooter(g, W, H, meta);
    var a = document.createElement('a'); a.href = cv.toDataURL('image/png'); a.download = meta.graphNo + '.png'; a.click();
  }
  function downloadOffCalFormalPng(){
    if(!offLog.length){ alert('ยังไม่มีข้อมูลเส้นโค้งคาลิเบรตให้ส่งออก — เริ่มวัดที่ตัวเครื่องก่อน'); return; }
    var chartCv = document.createElement('canvas'); chartCv.width = 900; chartCv.height = 420;
    drawOffCalCurve(chartCv, 'formal');
    var meta = formalMeta('OFFCAL');
    var selText = byId('offCalZoneSel') ? byId('offCalZoneSel').options[byId('offCalZoneSel').selectedIndex].text : 'ทั้งหมด';
    var W = chartCv.width + 120, H = chartCv.height + 280;
    var cv = document.createElement('canvas'); cv.width = W; cv.height = H; var g = cv.getContext('2d');
    drawFormalFrame(g, W, H, 'Live Calibration Curve (raw → aw)',
      'ช่วงที่ใช้ fit: ' + selText + ' — จุดสี่เหลี่ยม = aw เฉลี่ยที่วัดได้จริงต่อช่วง raw, เส้นแดง = สมการถดถอยเชิงเส้นที่ fit ได้จากจุดเหล่านั้น', meta);
    g.drawImage(chartCv, 60, 190);
    drawFormalFooter(g, W, H, meta);
    var a = document.createElement('a'); a.href = cv.toDataURL('image/png'); a.download = meta.graphNo + '.png'; a.click();
  }
  function downloadOffStatsFormalPng(){
    if(!offLog.length){ alert('ยังไม่มีข้อมูลตารางสรุปให้ส่งออก — เริ่มวัดที่ตัวเครื่องก่อน'); return; }
    var refV = parseFloat((byId('offRefInput').value || '').replace(',', '.')); var hasRef = !isNaN(refV);
    var names = ['Zone 0 — CaCl2/MgCl2', 'Zone 1 — MgCl2→NaCl', 'Zone 2 — NaCl→KCl', 'Zone 3 — KCl→Pure water'];
    var rows = [];
    for(var z = 0; z < 4; z++){
      var pts = offLog.filter(function(p){ return offZoneOf(p.z) === z; });
      if(!pts.length){ rows.push([names[z], '–', '0', '–', '–', '–', '–']); continue; }
      var rawArr = pts.map(function(p){ return p.raw; }), offArr = pts.map(function(p){ return p.off; });
      var reg = offLinRegress(pts.map(function(p){ return {x:p.raw, y:p.out}; }));
      var errArr = pts.map(function(p){ var t = hasRef ? refV : p.tgt; return Math.abs(p.out - t); });
      var meanErr = errArr.reduce(function(a,b){ return a+b; }, 0) / errArr.length;
      rows.push([names[z], Math.min.apply(null,rawArr).toFixed(4) + ' – ' + Math.max.apply(null,rawArr).toFixed(4), String(pts.length),
        reg ? reg.r2.toFixed(4) : '–', Math.min.apply(null,offArr).toFixed(3), Math.max.apply(null,offArr).toFixed(3), meanErr.toFixed(3)]);
    }
    var meta = formalMeta('OFFSTAT');
    var W = 1400, head = ['Zone', 'Raw range found', 'N (points)', 'R\u00B2 (raw vs aw)', 'Min offset', 'Max offset', 'Mean |error|'];
    var colW = [300, 260, 140, 220, 170, 170, 178];
    var tT = 200, rowH = 34, H = tT + 30 + rows.length * rowH + 130;
    var cv = document.createElement('canvas'); cv.width = W; cv.height = H; var g = cv.getContext('2d');
    drawFormalFrame(g, W, H, 'Offset Adjustment — Zone Quality Summary',
      'R² ของเส้นตรง raw→aw ในแต่ละโซน / ช่วง offset ที่ปรับจริง / |error| เฉลี่ย ' + (hasRef ? ('เทียบค่าจริงที่กรอกไว้ ' + refV.toFixed(4)) : 'เทียบเป้าหมายของโซนนั้น ๆ'), meta);
    var tL = 60, tW = colW.reduce(function(a,b){ return a+b; }, 0);
    g.font = '700 15px ' + FORMAL_FONT; g.fillStyle = '#111111'; g.textAlign = 'left';
    var cx = tL; head.forEach(function(h, i){ g.fillText(h, cx + 10, tT + 22); cx += colW[i]; });
    g.strokeStyle = '#111111'; g.lineWidth = 1.5; g.beginPath(); g.moveTo(tL, tT + 30); g.lineTo(tL + tW, tT + 30); g.stroke();
    rows.forEach(function(r, ri){
      var ry = tT + 30 + ri * rowH;
      g.strokeStyle = '#dddddd'; g.lineWidth = 1; g.beginPath(); g.moveTo(tL, ry + rowH); g.lineTo(tL + tW, ry + rowH); g.stroke();
      var cx2 = tL;
      r.forEach(function(v, ci){
        g.fillStyle = (ci === 0) ? '#111111' : '#333333'; g.font = ((ci === 0) ? '600 ' : '400 ') + '15px ' + FORMAL_FONT;
        g.fillText(String(v), cx2 + 10, ry + 23); cx2 += colW[ci];
      });
    });
    drawFormalFooter(g, W, H, meta);
    var a = document.createElement('a'); a.href = cv.toDataURL('image/png'); a.download = meta.graphNo + '.png'; a.click();
  }

  // ============================================================
  //  v-pro: ทำนายค่าสมดุลแบบ "สด" ฝั่งเว็บ — สำหรับกราฟที่ 2 (โหมดวัดมืออาชีพ + กราฟคาลิเบตของแอดมิน)
  // ============================================================
  // หลักการเดียวกับโหมด Predict บนตัวเครื่อง (เฉลี่ยเป็นบล็อก แล้วฟิตกำลังสองน้อยสุดแบบ AR(1): y[i+1] = A + B*y[i])
  // แต่คำนวณจากข้อมูลที่เว็บมีอยู่แล้วระหว่างกำลังวัด/คาลิเบตอยู่ ไม่ต้องไปวัดซ้ำอีกรอบที่เมนู "1.2 Predict"
  // ต่างหาก — นี่คือส่วนที่ทำให้ "ประหยัดเวลาในการใส่ข้อมูล" ตามที่ขอ เพราะรอบเดียวได้ทั้งกราฟจริง+กราฟทำนาย
  function proBlockAverage(pts, blocks) {
    if (!pts.length) return [];
    var t0 = pts[0].t, t1 = pts[pts.length - 1].t;
    var span = Math.max(1e-6, t1 - t0);
    var n = Math.max(1, Math.min(blocks, pts.length));
    var edges = []; for (var i = 0; i <= n; i++) edges.push(t0 + span * i / n);
    var out = [];
    for (i = 0; i < n; i++) {
      var sum = 0, c = 0, tsum = 0;
      pts.forEach(function (p) { if (p.t >= edges[i] && p.t <= edges[i + 1] + 1e-9) { sum += p.aw; tsum += p.t; c++; } });
      if (c) out.push({ t: tsum / c, aw: sum / c });
    }
    return out;
  }
  // v15: ถ่วงน้ำหนักตามความใหม่ (recency-weighted) แบบเดียวกับ fitAR1() ฝั่งเฟิร์มแวร์ (PRED_RECENCY_DECAY)
  // ให้คู่ (aw[i], aw[i+1]) ที่ใหม่กว่ามีอิทธิพลต่อการฟิตมากกว่าคู่เก่า — ตามพลวัตใกล้จุดสมดุลได้แม่นขึ้น
  var PRO_RECENCY_DECAY = 0.90;
  function proFitAR1(blk) {
    var n = blk.length;
    if (n < 4) return null;
    var m = n - 1;
    var w = []; for (var i = 0; i < m; i++) w.push(Math.pow(PRO_RECENCY_DECAY, m - 1 - i));
    var sw = 0, sx = 0, sy = 0;
    for (i = 0; i < m; i++) { sw += w[i]; sx += w[i] * blk[i].aw; sy += w[i] * blk[i + 1].aw; }
    if (sw < 1e-9) return null;
    var mx = sx / sw, my = sy / sw, sxx = 0, sxy = 0;
    for (i = 0; i < m; i++) { var dx = blk[i].aw - mx, dy = blk[i + 1].aw - my; sxx += w[i] * dx * dx; sxy += w[i] * dx * dy; }
    if (Math.abs(sxx) < 1e-12) return null;
    var B = sxy / sxx;
    var A = my - B * mx;
    if (!(B > 0 && B < 1)) return null;   // ไม่ลู่เข้าแบบเอ็กซ์โพเนนเชียล -> ไม่ทำนาย (ปลอดภัยไว้ก่อน)
    var eq = A / (1 - B);
    var dt = (blk[n - 1].t - blk[0].t) / (n - 1);
    if (dt <= 0) return null;
    var tau = -dt / Math.log(B);
    if (!(tau > 0)) return null;
    return { eq: eq, tau: tau, B: B, dt: dt };
  }
  // pts: [{t (นาที), aw}] เรียงเวลาแล้ว -> {eqAw, etaMin, confident, curve:[{t,aw}]}
  // curve ครอบคลุมตั้งแต่จุดแรกสุด (ย้อนหลัง ใช้ตรวจสอบว่าฟิตเข้ากับข้อมูลจริงดีแค่ไหน) ไปจนถึง ETA (อนาคต)
  function computeLivePrediction(pts) {
    var res = { eqAw: null, etaMin: null, confident: false, curve: [] };
    if (!pts || pts.length < 6) return res;
    var blocks = Math.max(8, Math.min(16, Math.floor(pts.length / 4)));  // v15: หน้าต่างบล็อกละเอียดขึ้น (เดิม 6-10) ให้ฟิตตามหางเส้นโค้งได้ไวขึ้น
    var blk = proBlockAverage(pts, blocks);
    var fit = proFitAR1(blk);
    if (!fit) return res;
    var last = pts[pts.length - 1];
    res.eqAw = fit.eq;
    var y0 = blk[0].aw;
    var remainNow = Math.abs(last.aw - fit.eq);
    var remainStart = Math.abs(y0 - fit.eq) || 1e-6;
    var fracNow = remainNow / remainStart;
    var tRemain = (fracNow > 0.05) ? fit.tau * Math.log(fracNow / 0.05) : 0;
    res.etaMin = last.t + Math.max(0, tRemain);
    res.confident = (pts.length >= 30) && fit.tau > 0 && fit.tau < 600;
    var t0 = pts[0].t;
    var curveEndT = Math.min(res.etaMin, last.t + Math.max(fit.tau * 4, 2));
    var steps = 50;
    for (var i = 0; i <= steps; i++) {
      var t = t0 + (curveEndT - t0) * i / steps;
      var aw = fit.eq + (last.aw - fit.eq) * Math.exp(-(last.t - t) / fit.tau);
      res.curve.push({ t: t, aw: aw });
    }
    return res;
  }
  function fmtEtaMin(min) {
    if (min === null || min === undefined || !isFinite(min) || min < 0) return '–';
    var h = Math.floor(min / 60), m = Math.round(min % 60);
    return (h > 0 ? (h + 'ชม.') : '') + m + 'นาที';
  }
  // วาดกราฟที่ 2 (ทำนาย) — ใช้ทั้งในโหมดวัดมืออาชีพ (proPredictChart) และกราฟคาลิเบตของแอดมิน (acalPredictChart)
  function drawPredictChart(canvasId, pts, pred, refAw, titleText) {
    var canvas = byId(canvasId);
    if (!canvas) return;
    var ctx = canvas.getContext('2d');
    var CW = canvas.width, CH = canvas.height;
    ctx.clearRect(0, 0, CW, CH);
    ctx.fillStyle = '#0e1822'; ctx.fillRect(0, 0, CW, CH);
    var padL = 60, padR = 20, padT = 26, padB = 30;
    var plotW = CW - padL - padR, plotH = CH - padT - padB;
    ctx.textAlign = 'left'; ctx.fillStyle = '#eaf2f8'; ctx.font = '700 13px sans-serif';
    ctx.fillText(titleText || 'กราฟทำนายค่าสมดุล (สด)', padL, 15);
    if (!pts || pts.length < 2) {
      ctx.fillStyle = '#8aa0b4'; ctx.font = '12px sans-serif';
      ctx.fillText('ยังไม่มีข้อมูลพอสำหรับทำนาย', padL, padT + plotH / 2);
      return;
    }
    var allVals = pts.map(function (p) { return p.aw; });
    if (pred && pred.curve && pred.curve.length) pred.curve.forEach(function (p) { allVals.push(p.aw); });
    if (typeof refAw === 'number' && refAw) allVals.push(refAw);
    var yMin = Math.min.apply(null, allVals), yMax = Math.max.apply(null, allVals);
    var padY = Math.max(0.004, (yMax - yMin) * 0.18);
    yMin -= padY; yMax += padY;
    var tMin = pts[0].t;
    var tMax = (pred && pred.curve && pred.curve.length) ? pred.curve[pred.curve.length - 1].t : pts[pts.length - 1].t;
    if (tMax <= tMin) tMax = tMin + 1;
    function xOf(t) { return padL + (t - tMin) / (tMax - tMin) * plotW; }
    function yOf(v) { return padT + plotH - (v - yMin) / (yMax - yMin) * plotH; }
    ctx.strokeStyle = '#22303c'; ctx.fillStyle = '#8aa0b4'; ctx.font = '11px sans-serif'; ctx.lineWidth = 1;
    for (var g = 0; g <= 4; g++) {
      var val = yMin + (yMax - yMin) * g / 4, y = yOf(val);
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
      ctx.fillText(val.toFixed(4), 4, y + 4);
    }
    if (typeof refAw === 'number' && refAw) {
      ctx.strokeStyle = '#ffd54a'; ctx.setLineDash([6, 4]); ctx.lineWidth = 1.5;
      var yr = yOf(refAw);
      ctx.beginPath(); ctx.moveTo(padL, yr); ctx.lineTo(padL + plotW, yr); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = '#ffd54a'; ctx.font = '11px sans-serif'; ctx.fillText('เป้าหมาย ' + refAw.toFixed(4), padL + plotW - 110, yr - 4);
    }
    if (pred && pred.curve && pred.curve.length) {
      ctx.strokeStyle = '#ff9f40'; ctx.setLineDash([8, 5]); ctx.lineWidth = 2; ctx.beginPath();
      pred.curve.forEach(function (p, i) { var x = xOf(p.t), y = yOf(p.aw); if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); });
      ctx.stroke(); ctx.setLineDash([]);
    }
    ctx.strokeStyle = '#07d9ff'; ctx.lineWidth = 2; ctx.beginPath();
    pts.forEach(function (p, i) { var x = xOf(p.t), y = yOf(p.aw); if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); });
    ctx.stroke();
    if (pred && typeof pred.eqAw === 'number') {
      ctx.strokeStyle = '#4caf50'; ctx.setLineDash([4, 4]); ctx.lineWidth = 1.5;
      var ye = yOf(pred.eqAw);
      ctx.beginPath(); ctx.moveTo(padL, ye); ctx.lineTo(padL + plotW, ye); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = '#4caf50'; ctx.font = '11px sans-serif';
      ctx.fillText((pred.confident ? '' : '~ ') + 'สมดุล ' + pred.eqAw.toFixed(4), padL + 8, ye - 6);
    }
  }
  function updatePredictReadouts(prefix, pred) {
    var eqEl = byId(prefix + 'PredEqVal'), etaEl = byId(prefix + 'PredEtaVal'), confEl = byId(prefix + 'PredConfVal');
    if (eqEl) eqEl.textContent = (pred && typeof pred.eqAw === 'number') ? ((pred.confident ? '' : '~') + pred.eqAw.toFixed(4)) : '–';
    if (etaEl) etaEl.textContent = (pred && pred.etaMin !== null) ? fmtEtaMin(pred.etaMin) : '–';
    if (confEl) confEl.textContent = pred ? (pred.eqAw === null ? '–' : (pred.confident ? 'มั่นใจ' : 'ยังไม่มั่นใจ (~)')) : '–';
  }
  // v-pro: กราฟ "เปรียบเทียบกราฟทำนาย" — ซ้อนเส้นทำนาย (ที่คำนวณไว้ตอนบันทึกแต่ละรอบ) ของทุกรอบทดสอบที่ติ๊กแสดงอยู่
  // ใช้สี/ชื่อเดียวกับรายการ "เปรียบเทียบกราฟที่บันทึกไว้" ด้านบน เพื่อให้ดูคู่กันแล้วรู้ทันทีว่าเส้นไหนของรอบไหน
  function drawComparePredictChart(){
    var box = byId('comparePredictBox');
    var canvas = byId('comparePredictChart');
    if(!box || !canvas) return;
    var runsWithPred = savedRuns.filter(function(r){ return r.visible && r.pred && typeof r.pred.eqAw === 'number' && r.pred.curve && r.pred.curve.length; });
    box.style.display = runsWithPred.length ? 'block' : 'none';
    if(!runsWithPred.length) return;
    var ctx = canvas.getContext('2d');
    var CW = canvas.width, CH = canvas.height;
    ctx.clearRect(0,0,CW,CH); ctx.fillStyle = '#0e1822'; ctx.fillRect(0,0,CW,CH);
    var padL = 60, padR = 20, padT = 26, padB = 30;
    var plotW = CW - padL - padR, plotH = CH - padT - padB;
    ctx.textAlign = 'left'; ctx.fillStyle = '#eaf2f8'; ctx.font = '700 13px sans-serif';
    ctx.fillText('เปรียบเทียบกราฟทำนาย (' + runsWithPred.length + ' รอบ)', padL, 15);
    var allVals = [], tMin = Infinity, tMax = -Infinity;
    runsWithPred.forEach(function(r){
      r.pred.curve.forEach(function(p){ allVals.push(p.aw); tMin = Math.min(tMin, p.t); tMax = Math.max(tMax, p.t); });
    });
    if(tMax <= tMin) tMax = tMin + 1;
    var yMin = Math.min.apply(null, allVals), yMax = Math.max.apply(null, allVals);
    var padY = Math.max(0.004, (yMax - yMin) * 0.18);
    yMin -= padY; yMax += padY;
    function xOf(t){ return padL + (t - tMin) / (tMax - tMin) * plotW; }
    function yOf(v){ return padT + plotH - (v - yMin) / (yMax - yMin) * plotH; }
    ctx.strokeStyle = '#22303c'; ctx.fillStyle = '#8aa0b4'; ctx.font = '11px sans-serif'; ctx.lineWidth = 1;
    for(var g = 0; g <= 4; g++){
      var val = yMin + (yMax - yMin) * g / 4, y = yOf(val);
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(padL + plotW, y); ctx.stroke();
      ctx.fillText(val.toFixed(4), 4, y + 4);
    }
    runsWithPred.forEach(function(r){
      var lastRealT = r.points.length ? r.points[r.points.length - 1].t : r.pred.curve[0].t;
      // ส่วนที่มีข้อมูลจริงรองรับ (t <= lastRealT) วาดเส้นทึบ, ส่วนคาดการณ์ต่อไป (t > lastRealT) วาดเส้นประ
      ctx.strokeStyle = r.color; ctx.lineWidth = 2; ctx.beginPath();
      var started = false;
      r.pred.curve.forEach(function(p, i){
        if(p.t > lastRealT + 1e-9) return;
        var x = xOf(p.t), y = yOf(p.aw);
        if(!started){ ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
      });
      ctx.stroke();
      ctx.setLineDash([7,4]); ctx.lineWidth = 1.8; ctx.beginPath();
      started = false;
      r.pred.curve.forEach(function(p){
        if(p.t < lastRealT - 1e-9) return;
        var x = xOf(p.t), y = yOf(p.aw);
        if(!started){ ctx.moveTo(x, y); started = true; } else ctx.lineTo(x, y);
      });
      ctx.stroke(); ctx.setLineDash([]);
      // จุดค่าสมดุลที่ทำนายไว้ + ป้ายชื่อรอบ
      var ye = yOf(r.pred.eqAw);
      ctx.fillStyle = r.color; ctx.beginPath(); ctx.arc(xOf(tMax), ye, 3.5, 0, Math.PI*2); ctx.fill();
    });
    // ตำนาน (legend) มุมขวาบน
    var lw = 320, lh = 18 * runsWithPred.length + 10;
    var lx = padL + plotW - lw - 6, ly = padT + 6;
    ctx.fillStyle = 'rgba(6,12,18,0.78)'; ctx.fillRect(lx, ly, lw, lh);
    ctx.strokeStyle = '#28394a'; ctx.lineWidth = 1; ctx.strokeRect(lx, ly, lw, lh);
    runsWithPred.forEach(function(r, idx){
      var ry = ly + 16 + idx * 18;
      ctx.fillStyle = r.color; ctx.fillRect(lx + 8, ry - 9, 10, 10);
      ctx.textAlign = 'left'; ctx.font = '12px sans-serif'; ctx.fillStyle = '#dce6ef';
      ctx.fillText(r.name + ' — ทำนาย ' + (r.pred.confident ? '' : '~') + r.pred.eqAw.toFixed(4), lx + 24, ry);
    });
  }
  // บันทึกกราฟทำนาย (กราฟที่ 2) เป็น PNG พื้นขาวแบบเอกสารทางการ — ใช้ renderFormalChart เดิม ส่ง 2 ชุดข้อมูล
  // (เส้นจริง + เส้นโค้งทำนาย) ทำให้ได้สี/เส้นประ/ตารางสรุปแยกต่างหากจากกราฟที่ 1 โดยไม่ต้องแก้ renderFormalChart เลย
  function downloadPredictFormalGraph(graphPrefix, pts, pred, refAw, datasetLabel) {
    if (!pts || pts.length < 6) { alert('ข้อมูลยังน้อยเกินไปสำหรับทำนาย — วัด/คาลิเบตต่ออีกสักครู่ก่อนบันทึกกราฟทำนาย'); return; }
    if (pred.eqAw === null) { alert('ยังทำนายไม่ได้ (ค่ายังไม่เริ่มลู่เข้าชัดเจน) — วัดต่ออีกสักครู่แล้วลองใหม่'); return; }
    var d = deviceInfoCache || {}, f = getReportForm();
    var graphNo = 'AWG-' + graphPrefix + '-' + ymd(new Date()) + '-' + idSuffix(Date.now());
    var meta = {
      graphNo: graphNo, deviceName: d.deviceName || 'AW Meter', fwVersion: d.fwVersion || '', valueMode: d.valueMode || valueMode,
      sampleName: f.sampleName, sampleId: f.sampleId, customer: f.customer,
      datasetName: datasetLabel + ' — predicted eq. ' + (pred.confident ? '' : '~') + pred.eqAw.toFixed(4) + ', ETA ' + fmtEtaMin(pred.etaMin),
      operator: f.analyst, recordedAt: fmtLocal(new Date()), issuedAt: fmtLocal(new Date()), conditions: conditionsList()
    };
    var cv = renderFormalChart({
      series: [
        { name: 'Measured (live)', points: pts, refAw: refAw },
        { name: 'Predicted curve to equilibrium', points: pred.curve }
      ],
      meta: meta
    }, function (w, h) { var c = document.createElement('canvas'); c.width = w; c.height = h; return c; });
    var a = document.createElement('a');
    a.href = cv.toDataURL('image/png');
    a.download = graphNo + '.png';
    a.click();
  }

  // v15: ดาวน์โหลด "เปรียบเทียบกราฟทำนาย" (การ์ดกราฟที่ 2 ในแผงเปรียบเทียบรอบที่บันทึกไว้) เป็น PNG พื้นขาวแบบ
  // เอกสารทางการ — จุดที่ขาดไปก่อนหน้านี้ (มีปุ่มดาวน์โหลดครบทุกกราฟอื่นแล้ว ยกเว้นกราฟนี้) หนึ่งเส้นต่อหนึ่งรอบที่ติ๊ก
  // แสดงอยู่ ใช้ renderFormalChart เดิมเหมือนกราฟอื่นทุกอัน — ตารางสรุปท้ายกระดาษจะขึ้นค่าทำนาย (สมดุล) ของแต่ละรอบ
  // แทนค่าจริง (เพราะ points ที่ส่งเข้าไปคือเส้นโค้งที่ทำนาย ไม่ใช่ค่าที่วัดได้จริง — ระบุไว้ในชื่อรอบให้ชัดเจน)
  function downloadCompareFormalGraph() {
    var runsWithPred = savedRuns.filter(function (r) { return r.visible && r.pred && typeof r.pred.eqAw === 'number' && r.pred.curve && r.pred.curve.length; });
    if (!runsWithPred.length) { alert('ยังไม่มีรอบที่ติ๊กแสดงและมีค่าทำนายพร้อมส่งออก — ติ๊กแสดงรอบที่ต้องการในรายการด้านบนก่อน'); return; }
    var d = deviceInfoCache || {}, f = getReportForm();
    var graphNo = 'AWG-CMP-PRED-' + ymd(new Date()) + '-' + idSuffix(Date.now());
    var meta = {
      graphNo: graphNo, deviceName: d.deviceName || 'AW Meter', fwVersion: d.fwVersion || '', valueMode: d.valueMode || valueMode,
      sampleName: f.sampleName, sampleId: f.sampleId, customer: f.customer,
      datasetName: 'Predicted equilibrium curves (' + runsWithPred.length + ' saved runs)',
      operator: f.analyst, recordedAt: fmtLocal(new Date()), issuedAt: fmtLocal(new Date()), conditions: conditionsList()
    };
    var cv = renderFormalChart({
      series: runsWithPred.map(function (r) {
        return {
          name: r.name + ' — predicted eq. ' + (r.pred.confident ? '' : '~') + r.pred.eqAw.toFixed(4),
          points: r.pred.curve, refAw: r.refAw
        };
      }),
      meta: meta
    }, function (w, h) { var c = document.createElement('canvas'); c.width = w; c.height = h; return c; });
    var a = document.createElement('a');
    a.href = cv.toDataURL('image/png');
    a.download = graphNo + '.png';
    a.click();
  }

  // ============================================================
  //  ออกใบรายงานผลการทดสอบ (Test Report / COA) — เปิดแท็บใหม่จัดหน้าพร้อมพิมพ์เป็น PDF
  // ============================================================
  // v-report: ใช้ window.print() ของเบราว์เซอร์เอง (เลือกปลายทางเป็น "บันทึกเป็น PDF") แทนการฝังไลบรารีสร้าง
  // PDF บนบอร์ด ซึ่งหนักเกินไปสำหรับ ESP32 — วิธีนี้ได้ไฟล์ PDF จริงโดยไม่ต้องพึ่งอินเทอร์เน็ต/บริการภายนอกเลย
  function buildReportHtml(sourceId){
    var src = resolveSource(sourceId);
    if(!src){ alert('ไม่พบชุดข้อมูลที่เลือก'); return null; }
    var name = src.name, points = src.points, refAw = src.refAw, savedAt = src.savedAt, dataId = src.dataId;
    if(!points.length){ alert('ชุดข้อมูลนี้ยังไม่มีจุดวัดเลย — เริ่มบันทึกกราฟ หรือเลือกชุดอื่นก่อนออกรายงาน'); return null; }

    var last = points[points.length - 1];
    var first = points[0];
    var durationMin = Math.max(0, last.t - first.t);
    var d = deviceInfoCache || {};

    var sampleName = byId('rptSampleName').value.trim();
    var sampleId = byId('rptSampleId').value.trim();
    var customer = byId('rptCustomer').value.trim();
    var analyst = byId('rptAnalyst').value.trim() || (d.operator || '');
    var approver = byId('rptApprover').value.trim();

    var reportNo = 'AWR-' + new Date().toISOString().slice(0,10).replace(/-/g,'') + '-' + Date.now().toString(36).toUpperCase().slice(-5);
    var issuedAt = new Date().toLocaleString('th-TH', { hour12:false });
    var savedAtTxt = (function(){ try{ return new Date(savedAt).toLocaleString('th-TH', { hour12:false }); }catch(e){ return '–'; } })();
    var errPct = (typeof refAw === 'number' && !isNaN(refAw) && refAw !== 0) ? (((last.aw - refAw) / refAw) * 100) : null;

    // v12: ใบรายงานไม่แนบกราฟแล้ว — อ้างอิงเลขที่ไฟล์กราฟพื้นขาวที่ดาวน์โหลดแยก (ปุ่ม "ดาวน์โหลดกราฟอ้างอิง") แทน
    var graphNo = 'AWG-' + dataId;
    var stabTxt = (typeof d.stabToleranceAw === 'number')
      // v-cal-fix: อ้าง stabWindowSec (หน้าต่างตัดสินความนิ่งจริงตาม stabBuf) แทน stabGraphWindowPts/Sec เดิม
      ? ('ช่วงกว้าง aw ≤ ' + d.stabToleranceAw.toFixed(4) + ' ตลอด ' + (typeof d.stabWindowSec === 'number' ? d.stabWindowSec : '–') + ' วิล่าสุด' +
         (typeof d.stabSlopeMaxPerMin === 'number' ? (' และความชัน ≤ ' + d.stabSlopeMaxPerMin.toFixed(4) + ' aw/นาที (เฉลี่ย 2 นาที)') : '') +
         (d.minMeasureDurationSec ? (' และวัดอย่างน้อย ' + (d.minMeasureDurationSec >= 120 ? Math.round(d.minMeasureDurationSec / 60) + ' นาที' : d.minMeasureDurationSec + ' วินาที')) : ''))
      : '–';
    var preTxt = (typeof d.shtPreHeatSec === 'number') ? ('ฮีต ' + d.shtPreHeatSec + ' วิ + รอเย็น ' + d.shtPreCoolSec + ' วิ') : '–';
    var gradTxt = (valueMode === 'RAW') ? 'ไม่ใช้ (โหมด RAW)' :
      ((d.gradCorr ? ('เปิด — สูตร Magnus, ไม่ชดเชยส่วนต่าง ≤ ' + (typeof d.gradDeadbandC === 'number' ? d.gradDeadbandC.toFixed(2) : '0.50') + ' °C, offset เซนเซอร์ ' + (typeof d.shtOffsetC === 'number' ? d.shtOffsetC.toFixed(2) : '0.00') + ' °C') : 'ปิด'));
    var postTxt = (typeof d.shtPostHeatSec === 'number') ? ('ฮีต ' + d.shtPostHeatSec + ' วิ' + (d.shtPostCoolSec ? (' + รอเย็น ' + d.shtPostCoolSec + ' วิ') : '')) : '–';

    var rows = '';
    rows += '<tr><td>Water Activity (aw)</td><td>' + last.aw.toFixed(3) + '</td><td>–</td></tr>';
    if(typeof last.raw === 'number' && !isNaN(last.raw))
      rows += '<tr><td>ค่าดิบก่อนคาลิเบรต (raw)</td><td>' + last.raw.toFixed(4) + '</td><td>–</td></tr>';
    if(typeof last.temp === 'number' && last.temp > -50)
      rows += '<tr><td>อุณหภูมิขณะวัด</td><td>' + last.temp.toFixed(1) + '</td><td>°C</td></tr>';
    if(typeof refAw === 'number' && !isNaN(refAw))
      rows += '<tr><td>ค่าอ้างอิง (reference)</td><td>' + refAw.toFixed(3) + '</td><td>–</td></tr>';
    if(errPct !== null)
      rows += '<tr><td>%Error เทียบค่าอ้างอิง</td><td>' + errPct.toFixed(2) + '</td><td>%</td></tr>';

    var analystLine = analyst ? escapeHtml(analyst) : '.....................................................';
    var approverLine = approver ? escapeHtml(approver) : '.....................................................';

    return '<!doctype html><html lang="th"><head><meta charset="utf-8">' +
      '<title>ใบรายงานผลการทดสอบ ' + reportNo + '</title><style>' +
      '*{box-sizing:border-box;}' +
      'body{font-family:"Segoe UI",Tahoma,Arial,sans-serif;color:#1a1a1a;background:#fff;margin:0;padding:28px 34px;font-size:13px;line-height:1.55;}' +
      '.toolbar{text-align:right;margin-bottom:16px;}' +
      '.toolbar button{font-size:13px;padding:8px 16px;border-radius:8px;border:1px solid #999;background:#f4f4f4;cursor:pointer;margin-left:8px;}' +
      '.toolbar button.primary{background:#0e6b5e;color:#fff;border-color:#0e6b5e;}' +
      'h1{font-size:19px;margin:0 0 2px;}' +
      '.subtitle{font-size:11.5px;color:#666;margin-bottom:4px;}' +
      '.notice{background:#fff6df;border:1px solid #e6c766;color:#7a5c00;font-size:11.5px;padding:8px 12px;border-radius:6px;margin:10px 0 18px;}' +
      '.hdr-row{display:flex;justify-content:space-between;align-items:flex-start;border-bottom:2px solid #222;padding-bottom:10px;margin-bottom:14px;}' +
      '.meta-grid{display:grid;grid-template-columns:1fr 1fr;gap:3px 24px;font-size:12px;margin-bottom:16px;}' +
      '.meta-grid span.k{color:#555;display:inline-block;min-width:150px;}' +
      'section{margin-bottom:18px;}' +
      'section h2{font-size:13.5px;border-bottom:1px solid #ccc;padding-bottom:4px;margin:0 0 8px;}' +
      'table{width:100%;border-collapse:collapse;font-size:12px;}' +
      'table th,table td{border:1px solid #ccc;padding:6px 8px;text-align:left;}' +
      'table th{background:#f2f2f2;}' +
      '.sign-row{display:flex;gap:40px;margin-top:40px;}' +
      '.sign-box{flex:1;text-align:center;font-size:11.5px;}' +
      '.sign-line{border-top:1px solid #333;margin-top:46px;padding-top:6px;}' +
      'footer{margin-top:26px;font-size:10.5px;color:#777;border-top:1px solid #ccc;padding-top:8px;}' +
      '@media print{.toolbar{display:none;}body{padding:0 12mm;}}' +
      '</style></head><body>' +
      '<div class="toolbar"><button onclick="window.close()">ปิดแท็บนี้</button>' +
      '<button class="primary" onclick="window.print()">พิมพ์ / บันทึกเป็น PDF</button></div>' +
      '<div class="hdr-row"><div><h1>ใบรายงานผลการทดสอบ (Test Report)</h1>' +
      '<div class="subtitle">Water Activity (aw) Measurement — ' + escapeHtml(d.deviceName || 'AW Meter') + '</div></div>' +
      '<div style="text-align:right;font-size:12px;">เลขที่รายงาน: <b>' + reportNo + '</b><br>วันที่ออกรายงาน: ' + issuedAt + '</div></div>' +
      '<div class="notice">⚠ เอกสารนี้เป็นข้อมูลเพื่อการตรวจสอบย้อนกลับ (traceability) ที่สร้างจากอุปกรณ์ AW Meter เท่านั้น ' +
      'ไม่ใช่ใบรับรองผลที่ได้รับการรับรองมาตรฐาน ISO/IEC 17025 หรือ มผช. — สำหรับยื่นขอการรับรองอย่างเป็นทางการ ' +
      'ต้องผ่านห้องปฏิบัติการที่ได้รับการรับรองแยกต่างหาก</div>' +
      '<section><h2>ข้อมูลตัวอย่าง (Sample Information)</h2><div class="meta-grid">' +
      '<div><span class="k">ชื่อตัวอย่าง</span>' + escapeHtml(sampleName || '–') + '</div>' +
      '<div><span class="k">รหัสตัวอย่าง / Lot No.</span>' + escapeHtml(sampleId || '–') + '</div>' +
      '<div><span class="k">ลูกค้า / หน่วยงาน</span>' + escapeHtml(customer || '–') + '</div>' +
      '<div><span class="k">ชุดข้อมูลที่ใช้</span>' + escapeHtml(name) + '</div>' +
      '<div><span class="k">วันที่บันทึกชุดข้อมูล</span>' + savedAtTxt + '</div>' +
      '<div><span class="k">ระยะเวลาที่วัด</span>' + durationMin.toFixed(1) + ' นาที</div>' +
      '<div><span class="k">จำนวนจุดข้อมูล</span>' + points.length + ' จุด</div>' +
      '<div><span class="k">กราฟอ้างอิง (Graph Ref.)</span>' + graphNo + '</div>' +
      '</div></section>' +
      '<section><h2>วิธีทดสอบและเงื่อนไข (Test Method &amp; Conditions)</h2><div class="meta-grid">' +
      '<div><span class="k">เครื่องมือ/เฟิร์มแวร์</span>' + escapeHtml(d.fwVersion || '–') + '</div>' +
      '<div><span class="k">โหมดค่าที่รายงาน</span>' + escapeHtml(d.valueMode || valueMode) + '</div>' +
      '<div><span class="k">เซนเซอร์ความชื้น</span>' + escapeHtml(d.humiditySensor || '–') + '</div>' +
      '<div><span class="k">เซนเซอร์อุณหภูมิ</span>' + escapeHtml(d.tempSensor || '–') + '</div>' +
      '<div><span class="k">อุณหภูมิเป้าหมายขณะวัด</span>' + (typeof d.targetTempC === 'number' ? d.targetTempC.toFixed(1) : '–') + ' °C</div>' +
      '<div><span class="k">ช่วงอุณหภูมิที่คาลิเบรตไว้</span>' + (typeof d.calTempMinC === 'number' ? d.calTempMinC.toFixed(1) : '–') + '–' + (typeof d.calTempMaxC === 'number' ? d.calTempMaxC.toFixed(1) : '–') + ' °C</div>' +
      '<div><span class="k">เกณฑ์ความนิ่งของกราฟ</span>' + stabTxt + '</div>' +
      '<div><span class="k">ฮีตเตอร์เซนเซอร์ก่อนวัด</span>' + preTxt + '</div>' +
      '<div><span class="k">ฮีตเตอร์เซนเซอร์หลังวัด</span>' + postTxt + '</div>' +
      '<div><span class="k">ชดเชยอุณหภูมิ (ชิป SHT vs ตัวอย่าง)</span>' + gradTxt + '</div>' +
      '<div><span class="k">คาลิเบรตล่าสุดเมื่อ</span>' + escapeHtml(d.calibratedAtUtc || '–') + '</div>' +
      '</div></section>' +
      '<section><h2>ผลการทดสอบ (Results)</h2><table><thead><tr><th>รายการ</th><th>ผล</th><th>หน่วย</th></tr></thead>' +
      '<tbody>' + rows + '</tbody></table></section>' +
      '<section><h2>เอกสารอ้างอิงแนบ (Attached Reference)</h2><table><thead><tr><th>รายการ</th><th>เลขที่อ้างอิง</th><th>หมายเหตุ</th></tr></thead><tbody>' +
      '<tr><td>กราฟแนวโน้ม aw &amp; อุณหภูมิ (ไฟล์ PNG พื้นขาว)</td><td>' + graphNo + '</td><td>' + points.length +
      ' จุดข้อมูล — ดาวน์โหลดจากแดชบอร์ด ชื่อไฟล์ ' + graphNo + '.png</td></tr></tbody></table></section>' +
      '<section><h2>ผู้ปฏิบัติงาน (Sign-off)</h2><div class="sign-row">' +
      '<div class="sign-box">' + analystLine + '<div class="sign-line">ผู้ทดสอบ (Tested by)&nbsp;&nbsp;&nbsp;วันที่: ......../......../..........</div></div>' +
      '<div class="sign-box">' + approverLine + '<div class="sign-line">ผู้อนุมัติผล (Approved by)&nbsp;&nbsp;&nbsp;วันที่: ......../......../..........</div></div>' +
      '</div></section>' +
      '<footer>สร้างโดยระบบ AW Meter Dashboard — ' + escapeHtml(d.fwVersion || '') + ' — เลขที่รายงาน ' + reportNo + ' — ออกเมื่อ ' + issuedAt + '</footer>' +
      '</body></html>';
  }

  // ============================================================
  //  ผูกปุ่มทั้งหมด
  // ============================================================
  function wireButtons(){
    byId('recToggleBtn').onclick = function(){
      if(!recording){
        // v-web-ctrl: "เริ่มบันทึกกราฟ" บนเว็บ = สั่งเครื่องเริ่มวัดจริงทันที (ต้องล็อกอินผ่านมาแล้วถึงจะมาถึงหน้านี้ได้)
        if(measureStarting) return;
        measureStarting = true;
        var btn = byId('recToggleBtn');
        btn.disabled = true;
        fetch('/cmd/measure').then(function(r){
          if(!r.ok) return r.text().then(function(t){ throw new Error(t || ('HTTP ' + r.status)); });
          dataPoints.length = 0;
          liveDataId = null; liveStartedIso = null;
          recording = true;
          updateRecordUI();
          drawChart();
        }).catch(function(err){
          alert('สั่งเริ่มวัดไม่สำเร็จ: ' + (err && err.message ? err.message : 'เครื่องไม่ว่าง หรือยังไม่ได้ล็อกอิน'));
        }).finally(function(){
          measureStarting = false;
          btn.disabled = false;
        });
      } else {
        // v-web-ctrl: "หยุดบันทึก" บนเว็บ = สั่งเครื่องยกเลิกการวัดจริงด้วย (/cmd/cancel) ให้จอ/LCD บนตัวเครื่อง
        // ออกจากหน้าวัดกลับไปเมนู AW ทันที ไม่ใช่แค่หยุดเก็บข้อมูลฝั่งเบราว์เซอร์เฉย ๆ เหมือนเดิม
        recording = false;
        updateRecordUI();
        fetch('/cmd/cancel').catch(function(){ /* เงียบไว้ — อย่างน้อยฝั่งเว็บก็หยุดบันทึกแล้ว ต่อให้สั่งบอร์ดไม่สำเร็จ */ });
      }
    };

    byId('resetBtn').onclick = function(){
      dataPoints.length = 0;
      liveDataId = null; liveStartedIso = null;
      recording = false;
      updateRecordUI();
      drawChart();
    };

    byId('saveBtn').onclick = function(){
      if(dataPoints.length === 0){ alert('ยังไม่มีข้อมูลในกราฟปัจจุบันให้บันทึก'); return; }
      var defaultName = 'รอบทดสอบ ' + (savedRuns.length+1);
      var name = prompt('ตั้งชื่อกราฟนี้:', defaultName);
      if(name === null) return;
      var refInput = prompt('ใส่ค่า aw จริง (ค่าอ้างอิง) ถ้ามี เช่น 0.753 — เว้นว่างได้ถ้ายังไม่รู้ค่า:', '');
      var refAw = (refInput !== null && refInput.trim() !== '') ? parseFloat(refInput) : null;
      if(refAw !== null && isNaN(refAw)) refAw = null;
      // v-pro: คำนวณค่าทำนายสมดุลของรอบนี้ไว้ ณ ตอนบันทึกเลย เพื่อให้เอาไป "เปรียบเทียบกราฟทำนาย" ระหว่างหลายรอบได้ทีหลัง
      var predAtSave = computeLivePrediction(dataPoints.map(function(p){ return {t:p.t, aw:p.aw}; }));
      savedRuns.push({
        id: 'run_' + Date.now(),
        name: name.trim() || defaultName,
        color: nextColor(),
        visible: true,
        refAw: refAw,
        savedAt: new Date().toISOString(),
        points: dataPoints.map(function(p){ return {t:p.t, aw:p.aw, raw:p.raw, temp:p.temp, roomAw:p.roomAw, roomRaw:p.roomRaw}; }),
        pred: predAtSave
      });
      persistRuns(); renderRunList(); drawChart();
      alert('บันทึกกราฟแล้ว — ติ๊กเลือกในรายการด้านล่างเพื่อซ้อนเทียบกับกราฟอื่น');
    };

    byId('csvBtn').onclick = function(){
      if(dataPoints.length === 0){ alert('ยังไม่มีข้อมูล'); return; }
      var csv = buildMetaHeader('AW Meter - Live Run Log');
      csv += 'time_min,raw_aw,cal_aw,room_raw,room_aw,delta_from_room_aw,temp_c\n';
      dataPoints.forEach(function(p){
        var rawStr = (typeof p.raw === 'number' && !isNaN(p.raw)) ? p.raw.toFixed(5) : '';
        var roomRawStr = (typeof p.roomRaw === 'number' && !isNaN(p.roomRaw)) ? p.roomRaw.toFixed(5) : '';
        var roomAwStr = (typeof p.roomAw === 'number' && !isNaN(p.roomAw)) ? p.roomAw.toFixed(5) : '';
        var deltaStr = (typeof p.roomAw === 'number' && !isNaN(p.roomAw)) ? (p.aw - p.roomAw).toFixed(5) : '';
        csv += p.t.toFixed(3) + ',' + rawStr + ',' + p.aw.toFixed(3) + ',' + roomRawStr + ',' + roomAwStr + ',' + deltaStr + ',' + p.temp.toFixed(1) + '\n';
      });
      downloadBlob(csv, 'text/csv', 'aw_log_' + Date.now() + '.csv');
    };

    // v12: ส่งออกกราฟเป็น PNG พื้นขาวแบบเอกสารทางการ (เลขที่ AWG-…) — กราฟสดบนหน้าเว็บยังเป็นธีมมืดเหมือนเดิม
    byId('pngBtn').onclick = function(){ downloadFormalGraph(collectVisibleSources()); };

    // v15: ส่งออกกราฟ "เปรียบเทียบกราฟทำนาย" (ซ้อนหลายรอบ) เป็น PNG พื้นขาวแบบเอกสารทางการเช่นกัน
    var comparePredictPngBtnEl = byId('comparePredictPngBtn');
    if(comparePredictPngBtnEl) comparePredictPngBtnEl.onclick = downloadCompareFormalGraph;

    // v-pro: โหมดวัดมืออาชีพ — ปิดกราฟที่ 2 ทันทีเมื่อเลิกติ๊ก (ไม่ต้องรอโพลรอบถัดไป) + ปุ่มบันทึกกราฟทำนาย (กราฟที่ 2) แยกจากกราฟที่ 1
    var proModeToggleEl = byId('proModeToggle');
    if(proModeToggleEl) proModeToggleEl.onchange = function(){
      if(!this.checked){ var b = byId('proPredictBox'); if(b) b.style.display = 'none'; }
    };
    var proPredictPngBtnEl = byId('proPredictPngBtn');
    if(proPredictPngBtnEl) proPredictPngBtnEl.onclick = function(){
      var proPts = dataPoints.map(function(p){ return {t:p.t, aw:p.aw}; });
      var proPred = computeLivePrediction(proPts);
      downloadPredictFormalGraph('PRED', proPts, proPred, null, 'Professional mode — live measurement');
    };
    byId('graphBtn').onclick = function(){
      var src = resolveSource(byId('rptSource').value);
      if(!src){ alert('ไม่พบชุดข้อมูลที่เลือก'); return; }
      downloadFormalGraph([src]);
    };

    byId('reportBtn').onclick = function(){
      var html = buildReportHtml(byId('rptSource').value);
      if(!html) return; // buildReportHtml แจ้ง alert เหตุผลให้เองแล้วถ้าออกรายงานไม่ได้
      var win = window.open('', '_blank');
      if(!win){ alert('เบราว์เซอร์บล็อกการเปิดแท็บใหม่ — กรุณาอนุญาตป๊อปอัปสำหรับหน้านี้แล้วลองอีกครั้ง'); return; }
      win.document.open();
      win.document.write(html);
      win.document.close();
    };

    byId('clearAllBtn').onclick = function(){
      if(savedRuns.length === 0) return;
      if(!confirm('ลบกราฟที่บันทึกไว้ทั้งหมด? การกระทำนี้ย้อนกลับไม่ได้')) return;
      savedRuns = []; persistRuns(); renderRunList(); drawChart();
    };

    byId('exportRunsBtn').onclick = function(){
      if(savedRuns.length === 0){ alert('ยังไม่มีกราฟที่บันทึกไว้'); return; }
      var csv = buildMetaHeader('AW Meter - All Saved Runs (Raw Data Export)');
      csv += 'run_name,run_saved_at,ref_aw,time_min,raw_aw,cal_aw,room_raw,room_aw,delta_from_room_aw,temp_c\n';
      savedRuns.forEach(function(r){
        var refStr = (typeof r.refAw === 'number' && !isNaN(r.refAw)) ? r.refAw.toFixed(3) : '';
        r.points.forEach(function(p){
          var rawStr = (typeof p.raw === 'number' && !isNaN(p.raw)) ? p.raw.toFixed(5) : '';
          var roomRawStr = (typeof p.roomRaw === 'number' && !isNaN(p.roomRaw)) ? p.roomRaw.toFixed(5) : '';
          var roomAwStr = (typeof p.roomAw === 'number' && !isNaN(p.roomAw)) ? p.roomAw.toFixed(5) : '';
          var deltaStr = (typeof p.roomAw === 'number' && !isNaN(p.roomAw)) ? (p.aw - p.roomAw).toFixed(5) : '';
          csv += r.name.replace(/,/g,' ') + ',' + (r.savedAt||'') + ',' + refStr + ',' + p.t.toFixed(3) + ',' + rawStr + ',' + p.aw.toFixed(3) + ',' + roomRawStr + ',' + roomAwStr + ',' + deltaStr + ',' + p.temp.toFixed(1) + '\n';
        });
      });
      downloadBlob(csv, 'text/csv', 'aw_compare_all_' + Date.now() + '.csv');
    };

    byId('exportStatsBtn').onclick = function(){
      var rows = savedRuns.filter(function(r){ return typeof r.refAw === 'number' && !isNaN(r.refAw); });
      if(rows.length === 0){ alert('ยังไม่มีรอบทดสอบที่กรอก "ค่าจริง (อ้างอิง)" ไว้'); return; }
      var csv = buildMetaHeader('AW Meter - Summary Statistics & Error vs Reference');
      csv += 'run_name,run_saved_at,ref_aw,raw_aw_measured,cal_aw_measured,error_raw_pct,error_cal_pct\n';
      rows.forEach(function(r){
        var st = runStats(r);
        csv += r.name.replace(/,/g,' ') + ',' + (r.savedAt||'') + ',' + st.ref.toFixed(3) + ',' +
          (st.raw!==null?st.raw.toFixed(5):'') + ',' + (st.cal!==null?st.cal.toFixed(3):'') + ',' +
          (st.errRaw!==null?st.errRaw.toFixed(2):'') + ',' + (st.errCal!==null?st.errCal.toFixed(2):'') + '\n';
      });
      downloadBlob(csv, 'text/csv', 'aw_summary_stats_' + Date.now() + '.csv');
    };

    byId('liveToggle').addEventListener('change', drawChart);

    byId('opSaveBtn').onclick = function(){
      var name = byId('opInput').value.trim();
      fetch('/setop?name=' + encodeURIComponent(name)).then(function(){
        byId('opStat').textContent = name || '(ยังไม่ตั้ง)';
      }).catch(function(){ alert('ตั้งชื่อผู้ปฏิบัติงานไม่สำเร็จ (เชื่อมต่อบอร์ดไม่ได้)'); });
    };

    var calSaveBtn = byId('calSaveBtn'), calResetBtn = byId('calResetBtn');
    if(calSaveBtn) calSaveBtn.onclick = saveCalPoints;
    var calSugBtn = byId('calSuggestBtn');
    if(calSugBtn) calSugBtn.onclick = renderCalSuggestion;
    if(calResetBtn) calResetBtn.onclick = resetCalPoints;
    var quickCalBtn = byId('quickCalBtn');
    if(quickCalBtn) quickCalBtn.onclick = quickCal;   // v22: ปุ่มคาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก
    // v-slope-live: ฟังการพิมพ์ในช่อง raw/aw ทุกช่อง (event delegation เพราะช่องถูกสร้างใหม่ทุกครั้งที่ renderCalRows วาดตาราง)
    // แล้วอัปเดตช่อง "จุดที่ยืด/หดสเกล" ทันทีที่พิมพ์ ไม่ต้องรอกดบันทึกก่อนถึงจะเห็นว่าจุดไหนน่าสงสัย
    var calRowsWrap = byId('calRows');
    if(calRowsWrap) calRowsWrap.addEventListener('input', function(e){
      if(e.target && (e.target.classList.contains('calRaw') || e.target.classList.contains('calAw'))) renderCalSlopeBox();
    });

    // v-web-ctrl: แผงเฉพาะแอดมิน — สลับกราฟ RAW ซ้อน + เริ่ม/ยกเลิกโหมดคาลิเบตอัตโนมัติ
    var rawToggleEl = byId('rawToggle');
    if(rawToggleEl) rawToggleEl.addEventListener('change', drawChart);
    var yZoomEl = byId('yZoomToggle');
    if(yZoomEl) yZoomEl.addEventListener('change', drawChart);

    // v-acal-graph: ขั้นตอนแรกบังคับกรอกค่าเป้าหมาย (aw อ้างอิง 0-1) ก่อน ปุ่มเริ่มจะกดไม่ได้จนกว่าจะกรอกถูกต้อง
    var acalRefInputEl = byId('acalRefInput');
    function validateAcalRef(){
      var v = parseFloat(acalRefInputEl ? acalRefInputEl.value : '');
      var ok = !isNaN(v) && v > 0 && v < 1.05;
      acalRefAw = ok ? v : null;
      var startBtn = byId('acalStartBtn');
      var hint = byId('acalRefHint');
      if(startBtn) startBtn.disabled = !ok;
      if(hint) hint.style.color = ok ? '#4caf50' : '#ffb300';
      return ok;
    }
    if(acalRefInputEl) acalRefInputEl.addEventListener('input', validateAcalRef);

    byId('acalStartBtn').onclick = function(){
      if(!validateAcalRef()){
        alert('ต้องกรอกค่าเป้าหมาย (aw อ้างอิงของสารละลายมาตรฐาน) ในขั้นตอนแรกก่อน ตัวเลขระหว่าง 0 ถึง 1 เพื่อให้คาลิเบตได้ถูกต้อง');
        return;
      }
      if(!confirm('เริ่มโหมดคาลิเบตอัตโนมัติ 10 รอบ (วัด/ทดสอบสลับกัน ที่ 25/25/25/20/19 °C แต่ละรอบจบเมื่อค่านิ่ง) ด้วยค่าเป้าหมาย aw = ' + acalRefAw.toFixed(4) + ' เครื่องต้องว่างและจะเทลเทียร์ปรับอุณหภูมิเองตลอดกระบวนการ ดำเนินการต่อ?')) return;
      byId('acalStartBtn').disabled = true;
      acalGraphAw = [];
      byId('acalSaveBtn').style.display = 'none';
      byId('acalSaveHint').textContent = '';
      fetch('/admin/calmode/start?ref=' + encodeURIComponent(acalRefAw.toFixed(4))).then(function(r){
        if(!r.ok) return r.text().then(function(t){ throw new Error(t || ('HTTP ' + r.status)); });
        pollAcalStatus();
      }).catch(function(err){
        alert('เริ่มโหมดคาลิเบตอัตโนมัติไม่สำเร็จ: ' + (err && err.message ? err.message : 'ไม่ทราบสาเหตุ'));
      }).finally(function(){ validateAcalRef(); });
    };

    byId('acalCancelBtn').onclick = function(){
      if(!confirm('ยกเลิกโหมดคาลิเบตอัตโนมัติที่กำลังทำอยู่?')) return;
      fetch('/admin/calmode/cancel').then(pollAcalStatus).catch(function(){});
    };

    byId('acalSaveBtn').onclick = function(){
      // v-acal-graph: บันทึกผล 5 รอบเป็นจุดคาลิเบรต — ใช้ RAW เฉลี่ยของ 3 รอบแรก (25°C) เทียบกับค่าเป้าหมายที่กรอกไว้
      // ใส่ลงตาราง "จุดคาลิเบรต" ด้านล่างให้แอดมินตรวจก่อน แล้วค่อยกด "บันทึกจุดคาลิเบรต" ยืนยันอีกที (ไม่บันทึกลง NVS ทันที)
      var refRounds = acalLastResults.slice(0, 6).filter(function(r){ return r.done; });   // v16: 6 รอบ 25°C (วัด+ทดสอบ 3 คู่)
      if(!refRounds.length || acalRefAw === null){
        byId('acalSaveHint').textContent = 'ยังไม่มีผลรอบ 25°C ที่เสร็จสมบูรณ์ หรือยังไม่ได้กรอกค่าเป้าหมาย';
        return;
      }
      // v16: จุดที่เครื่องปรับ/แก้จนนิ่งแล้ว (acalLastPoints[0]) มาก่อน ถ้าไม่มีค่อยเฉลี่ยรอบ 25°C ตรง ๆ
      var p25 = acalLastPoints[0];
      var avgRaw = (p25 && typeof p25.raw === 'number') ? p25.raw : (refRounds.reduce(function(s, r){ return s + r.avgRaw; }, 0) / refRounds.length);
      // v-multi-sample: SD ของ RAW เฉลี่ยระหว่าง 3 รอบ 25°C — วัดความ "เสถียร/ทำซ้ำได้" ของตัวอย่างมาตรฐานตัวนี้
      var sampleSd = calSd(refRounds.map(function(r){ return r.avgRaw; }));
      acalSamples.push({ ref: acalRefAw, avgRaw: avgRaw, sd: sampleSd, savedAt: new Date().toISOString() });
      renderAcalSamplesTable();

      var pts = [];
      var existingRows = document.querySelectorAll('.calRaw');
      existingRows.forEach(function(el, i){ pts.push({ raw: parseFloat(el.value), aw: parseFloat(document.querySelectorAll('.calAw')[i].value) }); });
      pts.push({ raw: avgRaw, aw: acalRefAw });
      pts.sort(function(a, b){ return a.raw - b.raw; });
      renderCalRows(pts);
      var calCard = byId('calCard'); if(calCard) calCard.style.display = 'block';

      // v-acal-graph: 2 รอบที่เหลือ (20°C, 17°C) ไม่ได้ใช้สร้างจุดคาลิเบรตเพิ่ม (สารละลายเดียวกัน aw จริงไม่เปลี่ยนตามอุณหภูมิ)
      // แต่ใช้ "ตรวจสอบ" ว่าสูตรชดเชยอุณหภูมิ (Magnus) หลัง applyCal() ยังแม่นอยู่ไหมที่อุณหภูมิต่ำกว่าห้องปกติ
      var ACAL_VERIFY_TOL_PCT = 3.0; // เกินนี้ถือว่าน่าสงสัย ควรตรวจ SHT_MINUS_DS_OFFSET_C / สูตรชดเชยอุณหภูมิ
      var verifyMsgs = [];
      [6, 7, 8, 9].forEach(function(idx){
        var r = acalLastResults[idx];
        if(r && r.done){
          var errPct = ((r.avgAw - acalRefAw) / acalRefAw) * 100;
          var flag = (Math.abs(errPct) > ACAL_VERIFY_TOL_PCT) ? ' ⚠ เกินเกณฑ์ ±' + ACAL_VERIFY_TOL_PCT + '%' : ' ✓ อยู่ในเกณฑ์';
          verifyMsgs.push('รอบ ' + r.round + ' (' + r.targetC.toFixed(0) + '°C ' + (r.type === 'test' ? 'ทดสอบ' : 'วัด') + '): error ' + errPct.toFixed(2) + '%' + flag);
        } else {
          verifyMsgs.push('รอบ ' + (idx+1) + ': ยังไม่มีผล (ไม่ครบ 10 รอบ)');
        }
      });
      [1, 2].forEach(function(k){
        var pk = acalLastPoints[k];
        if(p25 && pk && typeof pk.raw === 'number' && typeof p25.raw === 'number')
          verifyMsgs.push('จุดที่ ' + pk.tempC.toFixed(0) + '°C: RAW ' + pk.raw.toFixed(4) + ' (เลื่อนจาก 25°C ' + (pk.raw - p25.raw >= 0 ? '+' : '') + (pk.raw - p25.raw).toFixed(4) + ') — เก็บไว้ใช้ชดเชยตามอุณหภูมิ');
      });

      var sdFlag = (sampleSd > ACAL_SD_STABLE_TOL) ? (' ⚠ SD เกินเกณฑ์ ±' + ACAL_SD_STABLE_TOL.toFixed(4) + ' — ตัวอย่างนี้ยังไม่นิ่งพอ แนะนำวัดสารละลายตัวนี้ซ้ำอีกชุด') : ' ✓ เสถียรดี';
      byId('acalSaveHint').innerHTML =
        'เพิ่มจุด RAW=' + avgRaw.toFixed(4) + ' -> aw=' + acalRefAw.toFixed(4) + ' ลงตาราง "จุดคาลิเบรต" ด้านล่างแล้ว (มาจากจุดที่เครื่องปรับจนนิ่งในรอบ 25°C, SD=' + sampleSd.toFixed(4) + sdFlag + ') — ตรวจสอบแล้วกด "บันทึกจุดคาลิเบรต" เพื่อยืนยัน<br>' +
        'บันทึกลงรายการ "ตัวอย่างมาตรฐานที่คาลิเบตสะสมไว้" ด้านล่างแล้วด้วย — เปลี่ยนค่าเป้าหมายด้านบนแล้วกด "เริ่มคาลิเบตอัตโนมัติ" ใหม่เพื่อคาลิเบตสารละลายมาตรฐานตัวถัดไปได้เลย<br>' +
        '<b>ผลที่ 20°C / 19°C (4 รอบท้าย):</b><br>' + verifyMsgs.join('<br>');
      calCard.scrollIntoView({behavior:'smooth'});
    };

    // v-multi-sample: ตารางสะสมตัวอย่างมาตรฐานที่คาลิเบตแล้วในเซสชันนี้ + ปุ่มล้างรายการ
    byId('acalSamplesClearBtn').onclick = function(){
      if(!acalSamples.length) return;
      if(!confirm('ล้างรายการตัวอย่างสะสม ' + acalSamples.length + ' ตัวอย่างที่บันทึกไว้ในหน้านี้? (ไม่กระทบจุดคาลิเบรตที่กดยืนยันบันทึกลงเครื่องไปแล้ว)')) return;
      acalSamples = [];
      renderAcalSamplesTable();
    };

    // v-acal-graph: ดาวน์โหลดข้อมูลกราฟคาลิเบตสด (รอบปัจจุบัน) เป็น CSV — รวมสรุปผลรายรอบต่อท้ายไฟล์เดียวกัน
    byId('acalCsvBtn').onclick = function(){
      if(!acalGraphAw.length){ alert('ยังไม่มีข้อมูลกราฟคาลิเบตให้ดาวน์โหลด — เริ่มโหมดคาลิเบตอัตโนมัติก่อน'); return; }
      var csv = buildMetaHeader('AW Meter - Auto-Calibration Live Graph Log');
      csv += 'elapsed_sec,cal_aw,ref_aw\n';
      acalGraphAw.forEach(function(v, i){
        csv += i + ',' + v.toFixed(4) + ',' + (acalRefAw !== null ? acalRefAw.toFixed(4) : '') + '\n';
      });
      csv += '\nround,target_c,done,avg_raw,avg_aw,avg_temp_c\n';
      acalLastResults.forEach(function(r){
        csv += r.round + ',' + r.targetC.toFixed(1) + ',' + (r.done ? 1 : 0) + ',' +
          (r.done ? r.avgRaw.toFixed(4) : '') + ',' + (r.done ? r.avgAw.toFixed(4) : '') + ',' + (r.done ? r.avgTempC.toFixed(2) : '') + '\n';
      });
      if(acalSamples.length){
        csv += '\nsample_no,target_aw,avg_raw,sd\n';
        acalSamples.forEach(function(s, i){ csv += (i + 1) + ',' + s.ref.toFixed(4) + ',' + s.avgRaw.toFixed(4) + ',' + s.sd.toFixed(4) + '\n'; });
      }
      downloadBlob(csv, 'text/csv', 'aw_autocal_log_' + Date.now() + '.csv');
    };

    // v-acal-graph: ดาวน์โหลดกราฟคาลิเบตสด (รอบปัจจุบัน) เป็น PNG พื้นขาวแบบเอกสารทางการ — ใช้ตัววาดเดียวกับกราฟวัดค่าหลัก
    byId('acalPngBtn').onclick = function(){
      if(!acalGraphAw.length){ alert('ยังไม่มีข้อมูลกราฟคาลิเบตให้ดาวน์โหลด — เริ่มโหมดคาลิเบตอัตโนมัติก่อน'); return; }
      var d = deviceInfoCache || {}, f = getReportForm();
      var graphNo = 'AWG-ACAL-' + ymd(new Date()) + '-' + idSuffix(Date.now());
      var meta = {
        graphNo: graphNo,
        deviceName: d.deviceName || 'AW Meter',
        fwVersion: d.fwVersion || '',
        valueMode: d.valueMode || valueMode,
        sampleName: f.sampleName, sampleId: f.sampleId, customer: f.customer,
        datasetName: 'Auto-calibration live graph (รอบปัจจุบัน, เป้าหมาย aw ' + (acalRefAw !== null ? acalRefAw.toFixed(4) : '–') + ')',
        operator: f.analyst,
        recordedAt: fmtLocal(new Date()),
        issuedAt: fmtLocal(new Date()),
        conditions: conditionsList()
      };
      var pts = acalGraphAw.map(function(v, i){ return { t: i / 60, aw: v }; });
      var cv = renderFormalChart({
        series: [{ name: 'Auto-calibration live', points: pts, refAw: acalRefAw }],
        meta: meta
      }, function(w, h){ var c = document.createElement('canvas'); c.width = w; c.height = h; return c; });
      var a = document.createElement('a');
      a.href = cv.toDataURL('image/png');
      a.download = graphNo + '.png';
      a.click();
    };

    // v-pro: บันทึกกราฟที่ 2 (ทำนาย) ของรอบคาลิเบตนี้แยกต่างหากจากกราฟที่ 1 (acalPngBtn ด้านบน)
    byId('acalPredictPngBtn').onclick = function(){
      var acalPts = acalGraphAw.map(function(v, i){ return { t: i / 60, aw: v }; });
      var acalPred = computeLivePrediction(acalPts);
      downloadPredictFormalGraph('ACAL-PRED', acalPts, acalPred, acalRefAw, 'Auto-calibration round');
    };

    // v-web-ctrl: ปุ่ม "ออกจากระบบ / สลับบัญชี" มุมขวาบน — ใช้สลับไปมาระหว่างบัญชี person และ admin ได้ทุกเมื่อ
    var logoutBtn = byId('logoutBtn');
    if(logoutBtn) logoutBtn.onclick = function(){
      if(!confirm('ออกจากระบบตอนนี้เลย? หน้าเว็บจะรีโหลดและถามชื่อบัญชี/รหัสผ่านใหม่อีกครั้ง')) return;
      doLogout();
    };
  }

  function downloadBlob(content, type, filename){
    var blob = new Blob([content], {type:type});
    var a = document.createElement('a');
    a.href = URL.createObjectURL(blob);
    a.download = filename;
    a.click();
  }

  // ============================================================
  //  v-web-ctrl: ออกจากระบบ / สลับบัญชี (person <-> admin)
  // ============================================================
  // หน้าเว็บนี้ล็อกอินด้วย HTTP Basic Auth ซึ่งเบราว์เซอร์จำ user/pass ไว้เองและไม่มีปุ่ม "ออกจากระบบ" มาตรฐาน
  // ให้เรียก — ต้องใช้ทริค: ยิง XHR ไปที่หน้าเดิมพร้อมใส่ user/pass มั่ว ๆ (ทำให้เบราว์เซอร์ "จำทับ" ค่าที่ถูก
  // แคชไว้เดิมด้วยค่าที่ใช้ไม่ได้จริง) แล้วโหลดหน้าใหม่ — บอร์ดจะตอบ 401 (เพราะ user/pass มั่วนั้นใช้ไม่ได้)
  // เบราว์เซอร์จึงเด้งกล่องล็อกอินขึ้นมาใหม่ให้กรอกบัญชีอื่นได้ (person หรือ admin) โดยไม่ต้องปิดเบราว์เซอร์เลย
  function doLogout(){
    var btn = byId('logoutBtn');
    if(btn){ btn.disabled = true; btn.textContent = 'กำลังออกจากระบบ...'; }
    var xhr = new XMLHttpRequest();
    try{
      xhr.open('GET', '/whoami?_logout=' + Date.now(), true, 'logout', 'logout-' + Date.now());
    }catch(e){}
    xhr.onloadend = function(){ location.reload(); };
    xhr.onerror = function(){ location.reload(); };
    try{ xhr.send(); }catch(e){ location.reload(); }
    // เผื่อเบราว์เซอร์บางตัวไม่ยิง onloadend ให้ (พบเป็นบางกรณีบน Safari/iOS) บังคับรีโหลดสำรองไว้
    setTimeout(function(){ location.reload(); }, 1200);
  }

  // ============================================================
  //  เริ่มทำงาน
  // ============================================================
  // v-web-ctrl: ถามบอร์ดว่าเราล็อกอินเป็นใคร (person/admin) แล้วเปิด/ซ่อน UI เฉพาะแอดมิน (กราฟคู่ + แผงคาลิเบตอัตโนมัติ)
  function loadRole(){
    fetch('/whoami').then(function(r){ return r.json(); }).then(function(d){
      userRole = d && d.role ? d.role : 'none';
      applyRoleUI();
    }).catch(function(){ /* เงียบไว้ — ถ้าถามไม่สำเร็จก็ถือว่าเป็นผู้ใช้ทั่วไป UI แอดมินจะซ่อนอยู่แล้วโดย default */ });
  }

  function applyRoleUI(){
    var rb = byId('roleBadge');
    if(userRole === 'admin'){
      rb.textContent = 'ADMIN';
      rb.style.display = 'inline-block';
      byId('rawToggleRow').style.display = 'flex';
      byId('proModeRow').style.display = 'flex';  // v-pro: โหมดวัดมืออาชีพ เลือกได้เฉพาะแอดมิน จากเว็บเท่านั้น
      byId('pidTuneCard').style.display = 'block';
      byId('offsetCard').style.display = 'block';   // v-trend-offset
      byId('adminCalCard').style.display = 'block';
      renderAcalTable();
      startAcalPolling();
    } else if(userRole === 'person'){
      rb.textContent = 'USER';
      rb.style.display = 'inline-block';
      // v-pro: กราฟที่ 2 (ทำนายค่าสมดุล) เดิมเห็นเฉพาะแอดมิน ตอนนี้เปิดให้ผู้ใช้ทั่วไปใช้ได้ด้วยตามที่ขอ
      byId('proModeRow').style.display = 'flex';
    }
  }

  // v-web-ctrl: สถานะ/ตารางผลของโหมดคาลิเบตอัตโนมัติ (เฉพาะแอดมิน) — โพลทุก 2 วิ ขณะการ์ดแอดมินแสดงอยู่
  var acalLastResults = [];
  var acalLastPoints = [];   // v16: จุดคาลิเบรตชั่วคราวต่อช่องอุณหภูมิ (25/20/19°C) จาก /admin/calmode/status
  function renderAcalTable(refAw){
    var tbody = byId('acalRoundsBody');
    if(!tbody) return;
    var rows = acalLastResults.length ? acalLastResults : [
      {round:1,targetC:25,type:'measure',done:false},{round:2,targetC:25,type:'test',done:false},{round:3,targetC:25,type:'measure',done:false},
      {round:4,targetC:25,type:'test',done:false},{round:5,targetC:25,type:'measure',done:false},{round:6,targetC:25,type:'test',done:false},
      {round:7,targetC:20,type:'measure',done:false},{round:8,targetC:20,type:'test',done:false},
      {round:9,targetC:19,type:'measure',done:false},{round:10,targetC:19,type:'test',done:false}
    ];
    tbody.innerHTML = rows.map(function(r){
      var errPct = (r.done && typeof refAw === 'number' && refAw) ? (((r.avgAw - refAw) / refAw) * 100) : null;
      var isTest = (r.type === 'test');
      var verdict = !r.done ? '–' : (!isTest ? '–' : (r.pass ? '✓ ผ่าน' : (r.fixed ? '⚠ ไม่ผ่าน → แก้จุดใหม่' : '⚠ ไม่ผ่าน')));
      var durTxt = r.done ? (Math.floor(r.durSec/60) + ':' + ('0' + (r.durSec%60)).slice(-2) + (r.timedOut ? ' ⚠ ไม่นิ่ง (timeout)' : '')) : '–';
      return '<tr><td>รอบ ' + r.round + '</td><td>' + (isTest ? 'ทดสอบ' : 'วัด') + '</td><td>' + r.targetC.toFixed(1) + ' °C</td>' +
        '<td>' + (r.done ? '✓ เสร็จแล้ว' : 'ยังไม่เสร็จ') + '</td>' +
        '<td>' + (r.done ? r.avgRaw.toFixed(4) : '–') + '</td>' +
        '<td>' + (r.done ? r.avgAw.toFixed(3) : '–') + '</td>' +
        '<td>' + (r.done ? r.avgTempC.toFixed(1) + ' °C' : '–') + '</td>' +
        '<td>' + (errPct !== null ? errPct.toFixed(2) + '%' : '–') + '</td>' +
        '<td>' + verdict + '</td><td>' + durTxt + '</td></tr>';
    }).join('');
  }

  // v-multi-sample: วาดตารางตัวอย่างมาตรฐานที่คาลิเบตสะสมไว้ในเซสชันนี้ พร้อม SD (ความเสถียร) ของแต่ละตัวอย่าง
  function renderAcalSamplesTable(){
    var tbody = byId('acalSamplesBody'), lbl = byId('acalSampleCountLbl');
    if(lbl) lbl.textContent = '(' + acalSamples.length + ' ตัวอย่าง)';
    if(!tbody) return;
    if(!acalSamples.length){ tbody.innerHTML = '<tr><td colspan="5" class="hint">ยังไม่มีตัวอย่างที่บันทึก</td></tr>'; return; }
    tbody.innerHTML = acalSamples.map(function(s, i){
      var stable = s.sd <= ACAL_SD_STABLE_TOL;
      return '<tr><td>' + (i + 1) + '</td><td>' + s.ref.toFixed(4) + '</td><td>' + s.avgRaw.toFixed(4) + '</td>' +
        '<td>' + s.sd.toFixed(4) + '</td>' +
        '<td style="color:' + (stable ? '#7be0a5' : '#ffb300') + ';">' + (stable ? '✓ เสถียร' : '⚠ ควรวัดซ้ำ') + '</td></tr>';
    }).join('');
  }

  // v-acal-graph: วาดกราฟสดของรอบที่กำลังวัด (aw คาลิเบรตแล้ว) เทียบเส้นประเป้าหมาย — ใช้สไตล์เดียวกับ drawPidChart
  function drawAcalChart(refAw){
    var canvas = byId('acalChart');
    if(!canvas) return;
    var ctx = canvas.getContext('2d');
    var CW = canvas.width, CH = canvas.height;
    ctx.clearRect(0, 0, CW, CH);
    ctx.fillStyle = '#0e1822';
    ctx.fillRect(0, 0, CW, CH);
    var padL = 60, padR = 20, padT = 26, padB = 30;
    var plotW = CW - padL - padR, plotH = CH - padT - padB;
    ctx.textAlign = 'left'; ctx.fillStyle = '#eaf2f8'; ctx.font = '700 13px sans-serif';
    ctx.fillText('กราฟ aw สดของรอบปัจจุบัน', padL, 15);
    if(!acalGraphAw.length){
      ctx.fillStyle = '#8aa0b4'; ctx.font = '12px sans-serif';
      ctx.fillText('ยังไม่มีข้อมูล — เริ่มโหมดคาลิเบตอัตโนมัติก่อน', padL, padT + plotH/2);
      return;
    }
    var vals = acalGraphAw.slice();
    if(typeof refAw === 'number' && refAw) vals = vals.concat([refAw]);
    var yMin = Math.min.apply(null, vals) - 0.01, yMax = Math.max.apply(null, vals) + 0.01;
    if(yMax - yMin < 0.02){ var mid = (yMax+yMin)/2; yMin = mid - 0.01; yMax = mid + 0.01; }
    function xOf(i){ return padL + (acalGraphAw.length > 1 ? i/(acalGraphAw.length-1) : 0) * plotW; }
    function yOf(v){ return padT + plotH - (v - yMin)/(yMax - yMin) * plotH; }
    ctx.strokeStyle = '#22303c'; ctx.fillStyle = '#8aa0b4'; ctx.font = '11px sans-serif'; ctx.lineWidth = 1;
    for(var g = 0; g <= 4; g++){
      var val = yMin + (yMax-yMin)*g/4, y = yOf(val);
      ctx.beginPath(); ctx.moveTo(padL, y); ctx.lineTo(padL+plotW, y); ctx.stroke();
      ctx.fillText(val.toFixed(4), 4, y+4);
    }
    if(typeof refAw === 'number' && refAw){
      ctx.strokeStyle = '#ffd54a'; ctx.setLineDash([6,4]); ctx.lineWidth = 1.5;
      var yr = yOf(refAw);
      ctx.beginPath(); ctx.moveTo(padL, yr); ctx.lineTo(padL+plotW, yr); ctx.stroke(); ctx.setLineDash([]);
      ctx.fillStyle = '#ffd54a'; ctx.font = '11px sans-serif'; ctx.fillText('เป้าหมาย ' + refAw.toFixed(4), padL+plotW-90, yr-4);
    }
    ctx.strokeStyle = '#07d9ff'; ctx.lineWidth = 2;
    ctx.beginPath();
    acalGraphAw.forEach(function(v, i){ var x = xOf(i), y = yOf(v); if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y); });
    ctx.stroke();
  }

  function pollAcalStatus(){
    fetch('/admin/calmode/status').then(function(r){ return r.json(); }).then(function(d){
      acalLastResults = d.results || [];
      acalLastPoints = d.points || [];
      acalGraphAw = d.graphAw || [];
      var refAw = (typeof d.refAw === 'number') ? d.refAw : acalRefAw;
      renderAcalTable(refAw);
      drawAcalChart(refAw);
      // v-pro: กราฟที่ 2 ของรอบคาลิเบต — ทำนายค่าสมดุลสดจากกราฟคาลิเบตด้านบน ให้เห็นแนวโน้มก่อนครบ 25 นาทีเต็ม
      try{
        var acalPts = acalGraphAw.map(function(v, i){ return { t: i / 60, aw: v }; });
        var acalPred = computeLivePrediction(acalPts);
        drawPredictChart('acalPredictChart', acalPts, acalPred, refAw, 'กราฟที่ 2: ทำนายค่าสมดุลของรอบนี้');
        updatePredictReadouts('acal', acalPred);
      }catch(e){ console.error('pollAcalStatus: กราฟทำนายพลาด', e); }
      var dot = byId('acalDot'), stat = byId('acalStatus'), startBtn = byId('acalStartBtn'), cancelBtn = byId('acalCancelBtn'), saveBtn = byId('acalSaveBtn');
      if(!dot || !stat) return;
      var running = (d.phase === 'cooling' || d.phase === 'measuring');
      dot.classList.toggle('on', running);
      startBtn.style.display = running ? 'none' : '';
      cancelBtn.style.display = running ? '' : 'none';
      if(saveBtn) saveBtn.style.display = (d.phase === 'done') ? '' : 'none';
      if(d.phase === 'idle'){
        stat.innerHTML = 'ยังไม่เริ่มโหมดคาลิเบตอัตโนมัติ — กรอกค่าเป้าหมายด้านบนแล้วกดเริ่ม';
      } else if(d.phase === 'cooling'){
        stat.innerHTML = 'รอบ ' + d.round + '/' + d.totalRounds + ' — กำลังปรับอุณหภูมิไปที่ ' + d.targetC.toFixed(1) + '°C (ตอนนี้ ' + d.currentTempC.toFixed(1) + '°C) — เป้าหมาย aw ' + (refAw ? refAw.toFixed(4) : '–');
      } else if(d.phase === 'measuring'){
        var mm = Math.floor(d.elapsedSec/60), ss = d.elapsedSec%60;
        var ledTxt = (d.holdPhase === 2) ? '🟢 นิ่งแล้ว' : (d.holdPhase === 1) ? '🟡 คงที่ (รอครบ)' : '🔴 ยังเคลื่อนที่';
        stat.innerHTML = 'รอบ ' + d.round + '/' + d.totalRounds + ' (' + (d.isVerify ? 'ทดสอบ' : 'วัด') + ') — ' + d.targetC.toFixed(1) + '°C ผ่านไป ' + mm + ':' + (ss<10?'0':'') + ss + ' น. ' + ledTxt + ' (จบรอบเมื่อนิ่ง สูงสุด ' + Math.round(d.maxRoundSec/60) + ' น.) — เป้าหมาย aw ' + (refAw ? refAw.toFixed(4) : '–');
      } else if(d.phase === 'done'){
        stat.innerHTML = 'คาลิเบตอัตโนมัติครบ ' + d.totalRounds + ' รอบแล้ว — ดูผลแต่ละรอบในตาราง แล้วกด "บันทึกผลเป็นจุดคาลิเบรต" ด้านล่าง';
      }
    }).catch(function(){});
  }

  function startAcalPolling(){
    if(acalPollTimer) return;
    pollAcalStatus();
    acalPollTimer = setInterval(pollAcalStatus, 2000);
  }

  function init(){
    wireButtons();
    initPidTunerControls(); // v-pid-tune: ปุ่ม/ช่องกรอกของแผงจูน PID (ซ่อนอยู่จนกว่า loadRole() จะรู้ว่าเป็นแอดมิน)
    renderAcalSamplesTable(); // v-multi-sample: แสดงตาราง "0 ตัวอย่าง" ตั้งแต่เปิดหน้า (ก่อน role/poll ใด ๆ)
    renderRunList();
    updateRecordUI();
    drawChart();
    loadDeviceInfo();
    loadRole(); // v-web-ctrl
    // ซิงก์เวลาจริงของเบราว์เซอร์ให้บอร์ด (audit trail) ครั้งเดียวตอนเปิดหน้า
    fetch('/clocksync?epoch=' + Math.floor(Date.now()/1000)).catch(function(){});
    poll();
  }

  if(document.readyState === 'loading'){
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }
})();
</script>
</body></html>

)rawliteral";

// ---------- ฮาร์ดแวร์ ----------
const int RED_PIN = 25, GREEN_PIN = 26, BLUE_PIN = 27; // ขา KY-016
const int BTN_UP = 32, BTN_DOWN = 33; // ปุ่มแยกภายนอก 2 ปุ่ม

const int DS18B20_PIN = 13; // ขา GPIO ที่ต่อสาย DQ ของเซนเซอร์วัดอุณหภูมิ DS18B20

// v8: เปลี่ยนจากโมดูลรีเลย์ (Active-LOW, เปิด-ปิดเต็มกำลังได้อย่างเดียว) มาเป็นโมดูลขับมอเตอร์ RB046
// (ดูจากรูปจริง: เป็นบอร์ดสวิตช์ MOSFET 2 ช่อง — แต่ละช่องมี N-MOSFET + สกรูเทอร์มินัลขับโหลดแยกกัน
// แต่ทั้ง 2 ช่องใช้ขา GND และขา TRIG/PWM ร่วมกันเส้นเดียว จึงสวิตช์พร้อมกันด้วยสัญญาณเดียว) เหมาะพอดีกับ
// งานนี้ที่ต้องการพินเดียวสั่งทั้งเทลเทียร์+พัดลมพร้อมกัน (เหมือนพินรีเลย์เดิม) เพียงแต่ตอนนี้ TRIG รับ PWM
// ปรับกำลังไฟได้ต่อเนื่องด้วย — ไม่ต้องมีขาทิศทาง (ไม่ใช่ H-bridge)
// ต่อสาย: GND ของบอร์ด RB046 -> GND ของ ESP32 / TRIG (หรือ PWM) -> GPIO 17 (พินเดิมที่ต่อรีเลย์อยู่แล้ว)
//         เทลเทียร์ -> สกรูเทอร์มินัลช่อง 1 / พัดลม -> สกรูเทอร์มินัลช่อง 2 (ทั้งคู่จะเปิด-ปิด/ปรับไฟพร้อมกัน)
const int PELTIER_PWM_PIN = 17; // ขา TRIG/PWM เส้นเดียวคุมทั้ง 2 ช่องของบอร์ด (พินเดิม ไม่ต้องเดินสายใหม่)
const int PELTIER_PWM_FREQ = 1000; // ความถี่ PWM (Hz) — 1kHz เหมาะกับโหลดแบบเทลเทียร์/พัดลม DC
const int PELTIER_PWM_RES  = 8;    // ความละเอียด PWM 8 บิต -> ค่า 0-255 (ตรงกับ peltierOutputPWM เดิม)
// PELTIER_PWM_CHANNEL: เอาออกแล้ว — core 3.x ledcAttach()/ledcWrite() อ้างอิงด้วยเลขขา (pin) โดยตรง ไม่ต้องจัดการช่อง LEDC เอง

TFT_eSPI tft = TFT_eSPI();
// ---------- Native SHT45 driver (I2C, Sensirion SHT4x protocol) ----------
// SHT45 ไม่ใช่ SHT31: ใช้คำสั่ง 0xFD อ่านค่า high precision และตรวจ CRC8 ทุกเฟรม
class SHT45Compat {
 public:
  uint8_t addr = 0x44;
  bool begin(uint8_t a=0x44) { addr=a; Wire.beginTransmission(addr); return Wire.endTransmission()==0; }
  bool readBoth(float* tC, float* rh) {
    uint8_t cmd=0xFD; Wire.beginTransmission(addr); Wire.write(cmd);
    if (Wire.endTransmission()!=0) return false;
    delay(10);
    if (Wire.requestFrom((int)addr, 6)!=6) return false;
    uint8_t b[6]; for(int i=0;i<6;i++) b[i]=Wire.read();
    if (crc8(b,2)!=b[2] || crc8(b+3,2)!=b[5]) return false;
    uint16_t rt=((uint16_t)b[0]<<8)|b[1], rhv=((uint16_t)b[3]<<8)|b[4];
    *tC = -45.0f + 175.0f * ((float)rt / 65535.0f);
    *rh = -6.0f + 125.0f * ((float)rhv / 65535.0f);
    return !isnan(*tC) && !isnan(*rh);
  }
  float readHumidity(){ float t,h; return readBoth(&t,&h)?h:NAN; }
  float readTemperature(){ float t,h; return readBoth(&t,&h)?t:NAN; }
  void heater(bool on) {
    // SHT4x heater commands: high-power 1 s = 0x39 0x32. Off is implicit after command.
    if(!on) return;
    uint8_t c=0x39; Wire.beginTransmission(addr); Wire.write(c); Wire.endTransmission();
  }
 private:
  static uint8_t crc8(const uint8_t* d, uint8_t n) {
    uint8_t c=0xFF;
    for(uint8_t i=0;i<n;i++){ c^=d[i]; for(uint8_t b=0;b<8;b++) c=(c&0x80)?(uint8_t)((c<<1)^0x31):(uint8_t)(c<<1); }
    return c;
  }
};
SHT45Compat sht;  // ใช้ชื่อเดิมเพื่อไม่ต้องแก้จุดเรียกทั้งไฟล์

// ---------- v17: เซนเซอร์ความชื้นสำรอง DHT22/DHT11 (ขาเดียว GPIO15) ----------
// ผู้ใช้ถอดสลับ SHT45 (I2C, ขา 21/22) กับ DHT22/DHT11 (สายเดียว, ขา 15) ใช้แทนกันได้ — ต่อโมดูลไหนก็ต่อขาไหน
// บอร์ดตรวจจับเองตอนบูตว่ากำลังต่อตัวไหนอยู่ (ดู detectHumiditySensor() ใกล้ๆ readRawAw() ด้านล่าง) ไม่ต้องแก้โค้ด/
// #define สลับมือทุกครั้งที่เปลี่ยนเซนเซอร์ ผลตรวจจับจะโชว์ทั้งบนจอ (System Health) และเว็บ (ฟิลด์ "เซนเซอร์ความชื้น")
#define DHT_PIN 15
#define DHT_TYPE DHT22   // ถ้าใช้รุ่น DHT11 (ความละเอียด/แม่นยำต่ำกว่า DHT22) ให้เปลี่ยนบรรทัดนี้เป็น DHT11 แล้วอัปโหลดใหม่
DHT dht(DHT_PIN, DHT_TYPE);

enum HumiditySensorType { HUMSENS_NONE, HUMSENS_SHT, HUMSENS_DHT };
HumiditySensorType humSensorType = HUMSENS_NONE;   // ตั้งค่าจริงใน detectHumiditySensor() ที่เรียกจาก setup()
const char* humSensorTypeName() {
  return (humSensorType == HUMSENS_SHT) ? "SHT" : (humSensorType == HUMSENS_DHT) ? "DHT" : "NONE";
}

// ---------- ฮีตเตอร์ในตัวของเซนเซอร์ SHT45 ----------
// เป็นฮีตเตอร์เล็ก ๆ ที่ฝังอยู่ในตัวชิปเซนเซอร์เอง (ไม่ใช่ฮีตเตอร์ภายนอก) ใช้สำหรับ:
//   1) ไล่ไอน้ำ/หยดน้ำที่อาจเกาะบนผิวเซนเซอร์ในสภาวะความชื้นสูงมาก (ป้องกันค่าค้าง/อ่านผิดพลาด)
//   2) ตรวจสอบสุขภาพเซนเซอร์ (self-test) โดยเทียบค่าที่อ่านได้ก่อน/หลังเปิดฮีตเตอร์
// คำเตือน: ขณะฮีตเตอร์เปิดอยู่ ค่าอุณหภูมิ/ความชื้นที่อ่านได้จากเซนเซอร์ตัวนี้จะไม่แม่นยำ (ตัวชิปร้อนขึ้นเอง)
// จึงไม่ควรเปิดค้างไว้ระหว่างการวัดค่า aw จริง ให้เปิดเฉพาะตอนต้องการไล่ความชื้นสะสม แล้วปิดก่อนวัดค่า
bool sensorHeaterOn = false;
unsigned long lastShtHeaterPulseMs = 0;
void setSensorHeater(bool on) {
  sensorHeaterOn = on;
  lastShtHeaterPulseMs = millis();
  // v17: DHT22/DHT11 ไม่มีฮีตเตอร์ในตัวชิปแบบ SHT45 — สั่ง sht.heater() ต่อเมื่อกำลังใช้ SHT จริงเท่านั้น
  // (ยังคงปล่อยให้รอบฮีต/รอเย็นของ startSensorConditioning() เดินตามเวลาเดิมทุกประการแม้ไม่มีฮีตเตอร์จริงให้สั่ง
  // เพื่อไม่ต้องแก้ลำดับขั้นตอนวัดค่า/หน้าจออื่น — แค่ไม่มีผลจริงกับตัวเซนเซอร์ระหว่างนั้นถ้าใช้ DHT อยู่)
  if (humSensorType == HUMSENS_SHT) sht.heater(on);
}

// ---------- v12: โปรแกรมฮีตเตอร์เซนเซอร์ SHT ก่อน/หลังวัด (sensor conditioning) ----------
// ทำไมต้องมี: ตัวอย่างที่ชื้นสูงทิ้งไอน้ำ/ฟิล์มน้ำไว้บนผิวเซนเซอร์ (hysteresis) ทำให้การวัดรอบถัดไปเริ่มจากค่าที่เพี้ยน
//   - ก่อนวัด : ฮีตสั้น ๆ ไล่ความชื้นสะสม แล้ว "รอเย็น" ให้ตัวชิปกลับมาเท่าอุณหภูมิห้อง (ขณะร้อน %RH จะอ่านต่ำกว่าจริง)
//                ต้องรอเย็นเสร็จก่อนจึงเริ่มจับเวลา/เก็บกราฟ/ตัดสินความนิ่งจริง
//   - หลังวัด : ฮีตไล่ไอน้ำจากตัวอย่างที่เพิ่งวัด ก่อนกลับเมนู (ไม่ต้องรอเย็นต่อ เพราะรอบวัดถัดไปมี "ก่อนวัด" ของตัวเองอยู่แล้ว)
// แก้เวลาได้ที่ 4 บรรทัดนี้ (หน่วย ms) — ตั้งเป็น 0 เพื่อปิดขั้นตอนนั้น ๆ  (ค่าฮีต 8 วิ = เท่ากับ burst ของ auto heater เดิม)
const unsigned long SHT_PREHEAT_MS  = 15000UL;    // ก่อนวัด: เวลาที่เปิดฮีตเตอร์ SHT
const unsigned long SHT_PRECOOL_MS  = 90000UL;   // ก่อนวัด: เวลารอให้เซนเซอร์เย็นลงหลังปิดฮีตเตอร์
const unsigned long SHT_POSTHEAT_MS = 15000UL;    // หลังวัด: เวลาที่เปิดฮีตเตอร์ SHT
const unsigned long SHT_POSTCOOL_MS = 0UL;       // หลังวัด: เวลารอเย็น (ปกติไม่จำเป็น)

// ---------- ฮีตเตอร์อัตโนมัติ (auto heater) ----------
// ให้เฟิร์มแวร์ตัดสินใจเปิดฮีตเตอร์สั้นๆ เองโดยไม่ต้องรอผู้ใช้กดปุ่ม เพื่อไล่ไอน้ำ/หยดน้ำที่อาจเกาะ
// บนผิวเซนเซอร์ (โดยเฉพาะตอนวัดตัวอย่างที่มี aw สูงในกล่อง/ห้องปิด) และป้องกันเซนเซอร์ดริฟท์ระยะยาว
// กติกา (ปรับตัวเลขได้ตรงนี้ที่เดียว):
//   1) ห้ามฮีตขณะกำลังวัดค่า aw จริงอยู่ (ST_MEASURE_AW) หรือช่วง boot ที่เซนเซอร์ยังไม่นิ่ง — กันไม่ให้ผลวัดเพี้ยน
//   2) ห้ามยุ่งกับฮีตเตอร์ถ้าผู้ใช้เปิดเองผ่านเว็บ (manual) — auto จะจัดการเฉพาะครั้งที่ตัวเองเป็นคนสั่งเปิดเท่านั้น
//   3) เงื่อนไขที่จะสั่งฮีต:
//        - %RH อ่านได้ >= HEATER_HIGH_RH_THRESHOLD ต่อเนื่องนาน >= HEATER_HIGH_RH_SUSTAIN_MS (เสี่ยงหยดน้ำเกาะ)
//        - หรือไม่ได้ฮีตมานานเกิน HEATER_MAINT_INTERVAL_MS แล้ว และเครื่องว่างอยู่ที่เมนูหลัก (ฮีตป้องกันดริฟท์ตามระยะ)
//   4) แต่ละครั้งฮีตสั้นๆ แค่ HEATER_AUTO_BURST_MS แล้วปิดเองอัตโนมัติ และเว้นอย่างน้อย HEATER_AUTO_COOLDOWN_MS
//      ก่อนจะฮีตซ้ำ กันไม่ให้ฮีตถี่เกินไปจนรบกวนค่าที่อ่านได้บ่อยครั้ง
bool autoHeaterEnabled = true;                                    // เปิด/ปิดระบบอัตโนมัติทั้งหมด (ปรับผ่านเว็บได้)
bool heaterAutoActive = false;                                    // true = ฮีตเตอร์ที่เปิดอยู่ตอนนี้เป็นฝีมือ auto (ไม่ใช่ manual)
unsigned long heaterAutoOnAt = 0;                                 // เวลาที่ auto สั่งเปิดฮีตเตอร์ครั้งล่าสุด
unsigned long heaterLastAutoFireMs = 0;                           // เวลาที่ auto ฮีตเสร็จครั้งล่าสุด (ใช้คุม cooldown)
unsigned long highRhStartMs = 0;                                  // เวลาที่เริ่มพบ %RH สูงต่อเนื่อง (0 = ยังไม่พบ)
const unsigned long HEATER_AUTO_BURST_MS = 8000UL;                // ฮีตครั้งละ 8 วินาที
const unsigned long HEATER_AUTO_COOLDOWN_MS = 10UL * 60UL * 1000UL;   // เว้นอย่างน้อย 10 นาทีระหว่างการฮีตแต่ละครั้ง
const float HEATER_HIGH_RH_THRESHOLD = 97.0;                      // %RH ที่ถือว่าเสี่ยงหยดน้ำเกาะผิวเซนเซอร์
const unsigned long HEATER_HIGH_RH_SUSTAIN_MS = 3UL * 60UL * 1000UL;  // ต้องสูงต่อเนื่อง 3 นาทีถึงจะถือว่าเสี่ยงจริง
const unsigned long HEATER_MAINT_INTERVAL_MS = 2UL * 60UL * 60UL * 1000UL; // ฮีตป้องกันดริฟท์ทุก 2 ชม.ถ้าเครื่องว่าง
unsigned long lastAutoHeaterCheckMs = 0;
const unsigned long AUTO_HEATER_CHECK_INTERVAL_MS = 5000UL;       // เช็คเงื่อนไขทุก 5 วินาที (ไม่ต้องอ่านถี่เกินจำเป็น)

// หมายเหตุ: นิยามฟังก์ชัน updateAutoHeater() จริง ๆ อยู่ด้านล่างของไฟล์ (ใกล้ handleData())
// เพราะต้องใช้ตัวแปร state/enum AppState ซึ่งประกาศไว้ทีหลังในไฟล์นี้ — ใช้ prototype ด้านล่างแทน
// ============================================================================
//  v13: I2C แบบทนสัญญาณรบกวน + จอ LCD แบบ "SafeLCD"
// ============================================================================
// ปัญหาเดิม: จอ LCD 16x2 ผ่าน PCF8574 ไม่มีทางรู้เลยว่าข้อมูลที่ส่งไปเพี้ยนหรือไม่ (ไม่มี CRC) ถ้ามีสัญญาณรบกวน
// (PWM เทลเทียร์, ช่วงส่ง Wi-Fi, สายยาว) แม้แต่บิตเดียวก็ทำให้ขึ้นตัวอักษรมั่ว หรือจอหลุดโหมด 4-bit แล้วมั่วต่อเนื่อง
// เดิมโค้ดเขียน 32 ตัวอักษรใหม่ทุก 100 ms ทำให้มีโอกาสโดนรบกวนสูง — SafeLCD แก้ด้วย 4 วิธี:
//   1) เก็บ "สำเนาสิ่งที่จอควรแสดง" (shadow) แล้วส่งเฉพาะตัวอักษรที่เปลี่ยนจริง -> ทราฟฟิก I2C ลดลงมาก
//   2) ตัวอักษรนอกช่วง ASCII พิมพ์ได้ถูกแทนด้วย '?' (กันขยะในหน่วยความจำขึ้นจอ)
//   3) service(): เขียนทั้งสองบรรทัดซ้ำเป็นระยะ (LCD_REFRESH_MS) และ init จอใหม่เป็นระยะ (LCD_REINIT_MS) — ตัวเพี้ยนจะหายเอง
//   4) service(): เช็ค ACK ของจอ ถ้าไม่ตอบ 2 ครั้งติดกัน -> ล้างบัส I2C (i2cBusRecover) แล้ว init ใหม่
// API เหมือน LiquidCrystal_I2C เดิม (init/backlight/clear/setCursor/print) จึงไม่ต้องแก้จุดเรียกใช้ที่มีอยู่
uint32_t i2cRecoverCount = 0;
bool i2cStarted = false;

// ล้างบัส I2C: ถ้าอุปกรณ์ตัวใดค้างดึง SDA ต่ำ (เกิดได้เมื่อรีเซ็ตกลางการส่งข้อมูล) ให้ส่งพัลส์ SCL สูงสุด 9 ครั้งจนมันปล่อย
// แล้วส่งสภาวะ STOP ก่อนเริ่มบัสใหม่ — เป็นวิธีมาตรฐานของ I2C (ดู UM10204 ข้อ 3.1.16)
void i2cBusRecover() {
  if (i2cStarted) Wire.end();
  pinMode(I2C_SDA_PIN, INPUT_PULLUP);
  pinMode(I2C_SCL_PIN, OUTPUT);
  digitalWrite(I2C_SCL_PIN, HIGH);
  delayMicroseconds(10);
  for (int i = 0; i < 9 && digitalRead(I2C_SDA_PIN) == LOW; i++) {
    digitalWrite(I2C_SCL_PIN, LOW);  delayMicroseconds(10);
    digitalWrite(I2C_SCL_PIN, HIGH); delayMicroseconds(10);
  }
  pinMode(I2C_SDA_PIN, OUTPUT);
  digitalWrite(I2C_SDA_PIN, LOW);  delayMicroseconds(10);   // STOP = SDA ขึ้นขณะ SCL สูง
  digitalWrite(I2C_SCL_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(I2C_SDA_PIN, HIGH); delayMicroseconds(10);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(I2C_CLOCK_HZ);
  Wire.setTimeOut(I2C_TIMEOUT_MS);
  if (i2cStarted) i2cRecoverCount++;
  i2cStarted = true;
}

class SafeLCD {
 public:
  SafeLCD(uint8_t addr, uint8_t cols, uint8_t rows) : hw(addr, cols, rows), addr_(addr) { clearShadow(); }
  void init() {
    hw.init();
    Wire.setClock(I2C_CLOCK_HZ);   // hw.init() อาจเรียก Wire.begin() ซ้ำ -> ตั้งความเร็วซ้ำให้แน่ใจ
    hw.backlight();
    clearShadow();
    lastRefreshMs_ = lastReinitMs_ = lastProbeMs_ = millis();
  }
  void backlight() { hw.backlight(); }
  void clear() { hw.clear(); clearShadow(); col_ = 0; row_ = 0; }
  void setCursor(uint8_t c, uint8_t r) { col_ = c; row_ = (r < 2) ? r : 1; }
  size_t print(const char* s) { return s ? put(s, strlen(s)) : 0; }
  size_t print(const String& s) { return put(s.c_str(), s.length()); }
  size_t print(char c) { return put(&c, 1); }
  void requestReinit() { reinitPending_ = true; }
  void service();
  uint32_t reinitCount = 0;
  uint32_t probeFailCount = 0;
 private:
  LiquidCrystal_I2C hw;
  uint8_t addr_;
  char shadow_[2][17];
  uint8_t col_ = 0, row_ = 0;
  unsigned long lastRefreshMs_ = 0, lastReinitMs_ = 0, lastProbeMs_ = 0;
  int probeFailStreak_ = 0;
  bool reinitPending_ = false;
  void clearShadow() { for (int r = 0; r < 2; r++) { for (int c = 0; c < 16; c++) shadow_[r][c] = ' '; shadow_[r][16] = '\0'; } }
  void refreshAll() {
    for (int r = 0; r < 2; r++) { hw.setCursor(0, r); hw.print(shadow_[r]); }
    lastRefreshMs_ = millis();
  }
  void reinit() {
    hw.init();
    Wire.setClock(I2C_CLOCK_HZ);
    hw.backlight();
    refreshAll();                  // hw.init() ล้างจอ -> เขียนสำเนาเดิมกลับลงไป
    lastReinitMs_ = millis();
    reinitPending_ = false;
    reinitCount++;
  }
  size_t put(const char* s, size_t n) {
    if (col_ >= 16) return 0;
    if (n > (size_t)(16 - col_)) n = 16 - col_;
    char clean[17];
    int first = -1, last = -1;
    for (size_t i = 0; i < n; i++) {
      char ch = s[i];
      if (ch < 0x20 || ch > 0x7E) ch = '?';
      clean[i] = ch;
      if (shadow_[row_][col_ + i] != ch) { if (first < 0) first = (int)i; last = (int)i; }
    }
    if (first >= 0) {
      clean[last + 1] = '\0';      // ส่งเฉพาะช่วงที่มีตัวอักษรเปลี่ยน (ตั้งเคอร์เซอร์ครั้งเดียว)
      hw.setCursor(col_ + first, row_);
      hw.print(clean + first);
      for (int i = first; i <= last; i++) shadow_[row_][col_ + i] = clean[i];
    }
    col_ += (uint8_t)n;            // เลื่อนเคอร์เซอร์เหมือนจอจริง (โค้ดเดิมบางจุดพิมพ์ทีละตัวโดยไม่ setCursor)
    return n;
  }
};

void SafeLCD::service() {
  unsigned long now = millis();
  if (now - lastProbeMs_ >= I2C_PROBE_MS) {
    lastProbeMs_ = now;
    Wire.beginTransmission(addr_);
    uint8_t err = Wire.endTransmission();
    if (err != 0) {
      probeFailCount++;
      if (++probeFailStreak_ >= 2) {          // ไม่ตอบ 2 ครั้งติด -> ล้างบัส แล้ว init จอใหม่
        i2cBusRecover();
        reinitPending_ = true;
        probeFailStreak_ = 0;
      }
    } else {
      probeFailStreak_ = 0;
    }
  }
  if (reinitPending_ || (LCD_REINIT_MS > 0 && now - lastReinitMs_ >= LCD_REINIT_MS)) { reinit(); return; }
  if (now - lastRefreshMs_ >= LCD_REFRESH_MS) refreshAll();
}

SafeLCD lcd(0x27, 16, 2);
Preferences prefs;
OneWire oneWire(DS18B20_PIN);
DallasTemperature ds18b20(&oneWire);

int screenW, screenH, graphX, graphY, graphW, graphH;
const unsigned long DEBOUNCE_MS = 25;
const unsigned long BTN_HOLD_MS = 500; 

struct ButtonEvents {
  bool up, down, select, exit;
};
struct FoodCategory {
  float minVal, maxVal;
  const char* label;
  uint16_t color;
};
enum AppState { ST_BOOT_WARMUP,
                ST_MENU_MAIN,
                ST_MENU_AW,
                ST_MEASURE_AW,      // v12: เมื่อกราฟนิ่งจะมีป็อปอัป "Save?" ทับบนกราฟโดยยังวัดต่อ (แทนหน้า ST_MEASURE_DONE เดิม)
                ST_PREDICT_AW,      // v-predict: โหมดทำนายค่า aw สมดุลล่วงหน้า แยกจากการวัดปกติ
                ST_COMPARE_MEASURE, // v-compare: กำลังวัดตัวอย่าง A หรือ B (ดู compareStage) ในโหมดเปรียบเทียบ
                                    // v12: เมื่อกราฟนิ่งจะมีป็อปอัปถามทับบนกราฟโดยยังวัดต่อ (ดู savePromptActive)
                ST_COMPARE_RESULT,  // v-compare: หน้าสรุปผลเปรียบเทียบ A vs B หลังวัดครบทั้งคู่
                ST_SENSOR_COND,     // v12: กำลังฮีต/รอเย็นเซนเซอร์ SHT ก่อนหรือหลังวัด (ดู startSensorConditioning)
                ST_MENU_RECORD,
                ST_RECORD_ITEM_MENU,
                ST_RECORD_VIEW,
                ST_WIFI_INFO,
                ST_SYS_HEALTH }; // v-pro: หน้าสรุปสุขภาพระบบรวมศูนย์ (เซนเซอร์/Wi-Fi/นาฬิกา/อายุคาลิเบรต)
enum IconType { ICON_MEASURE,
                ICON_CAL,
                ICON_CANCEL,
                ICON_DISTILLED,
                ICON_SALT,
                ICON_DESICCANT,
                ICON_RECORD,
                ICON_DELETE,
                ICON_WIFI,
                ICON_PREDICT,
                ICON_COMPARE,
                ICON_HEALTH };
// v12: หลังจบขั้นตอนฮีต/รอเย็นแล้วให้ไปทำอะไรต่อ
enum CondNext { COND_NEXT_MEASURE,    // เริ่มวัดปกติ (1.1 Start)
                COND_NEXT_PREDICT,    // เริ่มโหมดทำนาย (1.2 Predict)
                COND_NEXT_COMPARE_A,  // เริ่มวัดตัวอย่าง A (1.3 Compare)
                COND_NEXT_COMPARE_B,  // เริ่มวัดตัวอย่าง B (หลังตอบ Yes ตอน A นิ่ง)
                COND_NEXT_MENU_AW };  // กลับเมนู AW (หลังวัดเสร็จ/ยกเลิก)
// v12: ข้อความของป็อปอัป "บันทึกไหม" (ดู currentPromptText()) — ประกาศ struct ไว้ตรงนี้ (ก่อน Forward declarations) เพราะ prototype ต้องใช้ชนิดนี้
struct PromptText { const char* tag; const char* question; const char* opt0; const char* opt1; const char* lcd0; const char* lcd1; };
// v-web-ctrl: บทบาทผู้ใช้ที่ล็อกอินเข้าเว็บแดชบอร์ด (ดู checkAuth()) — person = ผู้ใช้ทั่วไป, admin = เห็นกราฟจริง+กราฟคาลิเบรตคู่กัน และมีโหมดคาลิเบตอัตโนมัติ
enum UserRole { ROLE_NONE, ROLE_USER, ROLE_ADMIN };
UserRole currentRole = ROLE_NONE;

struct BtnState {
  bool stable = HIGH;
  bool lastRaw = HIGH;
  unsigned long lastChangeTime = 0;
  unsigned long pressStart = 0;
  bool pressed = false;
  bool longFired = false;
  bool chorded = false;
};
BtnState btnUp, btnDown;
bool chordLongFired = false; 

// v-accuracy: ย้าย struct CalPoint มาไว้ตรงนี้ (ก่อนส่วน Forward declarations ด้านล่าง) เพราะ prototype ของ
// loadCalPoints()/saveCalPoints()/isCalPointsMonotonic() ต้องใช้ชนิด CalPoint นี้ — นิยามตัวแปร/อาร์เรย์จริง
// (CAL_POINTS_FACTORY, calPoints) ยังอยู่ที่เดิมใกล้ applyCal() เหมือนเดิม ย้ายมาเฉพาะตัว struct เท่านั้น
struct CalPoint {
  float raw;  // เศษส่วน RH ดิบ 0-1
  float aw;   // ค่า aw อ้างอิงที่ต้องการให้แม็พไปถึง
};

// ---------- Forward declarations ----------
// เพิ่ม prototype ของฟังก์ชันทั้งหมดไว้ตรงนี้อย่างชัดเจน เพราะ Arduino IDE
// จะสร้าง prototype อัตโนมัติจากการสแกนไฟล์ (ใช้ ctags) ซึ่งบางครั้งสับสนกับ
// เนื้อหาภายใน R"rawliteral(...)rawliteral" ของหน้าเว็บ ทำให้ prototype ของ
// ฟังก์ชันที่อยู่หลังจุดนั้นหายไปและ compile ไม่ผ่าน (เช่น valueToY, drawCookieIcon)
bool updateButtonDebounce(BtnState& b, int pin, unsigned long now);
void setLED(bool r, bool g, bool b);
void blinkLED(bool r, bool g, bool b, unsigned long interval);
extern float currentTempC;
float applyCal(float raw, float tempC);
float roomReferenceAw(float tempC);
float gradientFactor();
void updateSubstanceGuess(float rawNow, unsigned long measureStartMsNow);   // v-substance-guess
float applySubstanceGuessBias(float awFromCal, float raw);                 // v-substance-guess
void resetSubstanceGuessWindow();
// v29 ADV
void advKfUpdate(float t);
void advKfReset();
bool advPidInputs(float& tHat, float& dTdt);
void advEkfFeed(float raw);
void advControlTick();
void advMlUpdate(const float* buf, int n, float arEqRaw);
void advMlClear();
void handleAdvStatus();
void handleAdvSet();
float awShownAtLock(float awCalLocked, float rawLocked);
float awShownPeek(float rawX, float tempC);
bool autoCalActive();   // v-raw-idle: true ระหว่างโหมดคาลิเบตอัตโนมัติ (ต้องแสดงค่าคาลิเบรตตามเดิม)
float awShownFromRaw(float raw, float tempC);                                          // v-substance-guess
bool updateLidEventDetector(float rawNow, unsigned long nowMs);            // v-lid-event
float gradientDeltaC();
bool isCalPointsMonotonic(const CalPoint* pts, int count);
void loadCalPoints();
bool saveCalPoints(const CalPoint* pts, int count, String& errOut);
void handleCalGet();
void handleCalSet();
void handlePidSet();   // v-pid-tune: GET /pidset?kp=..&ki=..&kd=.. ปรับเกน PID ของเทลเทียร์สดจากเว็บ ไม่ต้องคอมไพล์ใหม่
void handlePidAutoTuneStart();   // v20: GET /pidautotune?start=1 (หรือ ?cancel=1) — เริ่ม/ยกเลิกจูน PID อัตโนมัติ
void handlePidAutoTuneStatus();  // v20: GET /pidautotune/status — สถานะสดของการจูนอัตโนมัติ ให้เว็บโพลดู
void handleCalReset();
time_t nowEpoch();
void formatEpoch(time_t ep, char* out, size_t outSize);
void loadAmbientBaseline();     // Phase 1: โหลดค่าความชื้น/อุณหภูมิห้องที่เคยจับไว้ตอนบูตครั้งก่อน ๆ จาก NVS
void captureAmbientBaseline();  // Phase 1: จับค่าความชื้น/อุณหภูมิห้อง "ตอนนี้" (ฮีตเตอร์ปิด) แล้วบันทึกทับของเก่า
void loadLearnedOffset();               // Phase 2: โหลดค่า offset ที่เคยเรียนรู้สะสมไว้จาก NVS
void saveLearnedOffsetSample(float sampleOffsetC); // Phase 2: ปรับค่า offset ที่เรียนรู้ด้วยตัวอย่างใหม่ 1 ตัว (EMA) แล้วบันทึก
float gradientDeadbandC();               // v25: ดีดแบนด์ชดเชยอุณหภูมิที่ใช้จริง (ปรับตามความไม่แน่นอนของ offset ที่เรียนรู้)
float getEffectiveShtDsOffsetC();       // Phase 2: คืนค่า offset ที่ควรใช้จริง (เรียนรู้แล้ว ถ้าเชื่อถือได้ / ไม่งั้น fallback ค่าคงที่เดิม)
bool waitForDS18B20ConversionOnce(unsigned long timeoutMs); // Phase 2: รอ DS18B20 แปลงค่าเสร็จครั้งเดียวตอนบูต (ไม่บล็อกใน loop() ปกติ)
void loadClockFromNVS();
void syncClockFromWeb(time_t epochFromBrowser);
void saveOperatorTag(const char* tag);
void handleClockSync();
void handleSetOperator();
bool checkAuth();
void drawSysHealthScreen();
void drawIconHealth(int cx, int cy, int r);
bool isTempOutOfCalRange(float t);
float readRawAw();
float readAw();
float readAwAndRaw(float& rawOut);
void beginDS18B20();
void updateDS18B20();
void peltierOff();
void runPeltierBoostFull();
void runPeltierPID();
void updatePeltierControl();
// v20: PID Auto-Tune อัตโนมัติ (relay/Åström–Hägglund) + โปรไฟล์เกนแยกตามสภาพแวดล้อม — ดูคำอธิบายเต็มที่จุดนิยามจริง
bool loadPidProfileForEnv(float envC);
void savePidProfileForEnv(float envC, float kp, float ki, float kd);
bool startPidAutoTune(String &err);
void cancelPidAutoTune(bool restoreOldGains);
void runPidAutoTuneStep();
void updateColdRoomFlag();
// v-led-web / v-dist: ประกาศล่วงหน้า เพราะ handleData() (นิยามอยู่ก่อนหน้าฟังก์ชันจริงในไฟล์) เรียกใช้ทั้งคู่
const char* ledColorName();
bool getClientRssi(int8_t* outRssi);
float rssiToDistanceM(int8_t rssi);
void showWelcomeScreen();
void formatTempC(char* out, size_t outSize);
void formatElapsedTimeLCD(unsigned long startMs, char* out, size_t outSize);
void formatDurationShort(unsigned long durationSec, char* out, size_t outSize);
void drawBootWarmupStatic();
void drawBootWarmupDynamic();
void resetStabilityWindow();
void getStabRange(float& mn, float& mx);
void getStabTempRange(float& mn, float& mx);
void updateStabilityWindow(float v, float t);
void downsampleGraph(float* out, int n);
void saveRecording(float finalAw, int catIdx, unsigned long durationSec);
void loadRecordingIndex();
void loadRecordingSnapshot(int slot);
void deleteRecording(int viewIndex);
void drawRecordListScreen();
void drawRecordViewScreen();
float graphSteadyRange(int n);
int savePromptWindowPts();
const PromptText& currentPromptText();
void drawSensorCondStatic();
void drawSensorCondDynamic(unsigned long elapsedMs, unsigned long totalMs);
void refreshPromptValues();
float stabWindowMeanAw();
void drawPromptPopupTFT();
void drawPromptLCD();
void handleMeasurePromptChoice();
void handleComparePromptChoice();
void startSensorConditioning(bool isPre, CondNext next, int returnSel);
void runSensorCondTick(const ButtonEvents& e);
void finishSensorConditioning();
void cancelSensorConditioning();
const char* sensorPhaseName();
void beginMeasureAW();
void beginPredictAW();
void beginCompareA();
void beginCompareB();
void i2cBusRecover();
float readShtHumidityRetry();
void refreshShtTempOnly();  // Phase 3: อ่านเฉพาะอุณหภูมิชิป SHT อัปเดตสดระหว่างขั้น "รอเย็น" ให้ gradientDeltaC() ใช้ได้จริง
void shtBusRecover();
void peltierWrite(int target);
uint8_t pickWifiChannel();
bool startWifiAP(bool allowScan);
void wifiKeepAlive();
const char* resetReasonName(esp_reset_reason_t r);
void loadBootDiagnostics();
void bootStabilityService();
void jsonEscapeInto(const char* in, char* out, size_t outSize);
extern bool safeStart;                   // v13: นิยามจริงอยู่ใกล้ setup() (ส่วน Wi-Fi + บันทึกสาเหตุรีเซ็ต)
extern esp_reset_reason_t lastResetReason;
extern uint32_t bootBrownoutCount, bootWdtCount, bootPanicCount;
extern uint8_t wifiChannelInUse;
extern uint32_t wifiRestartCount;
void handleRoot();
void handleData();
void handleDeviceInfo();
void setSensorHeater(bool on);
void updateAutoHeater();
bool canStartMeasurement(const char** reasonOut);
void showStartBlockedWarning(const char* reason);
// v-web-ctrl: ล็อกอินเว็บ (HTTP Basic Auth) + สั่งบอร์ดเริ่ม/ยกเลิกวัดจากเว็บ + โหมดคาลิเบตอัตโนมัติสำหรับแอดมิน
void handleWhoAmI();
void handleCmdMeasure();
void handleCmdCancel();
void handleAdminCalStart();
void handleAdminCalStatus();
void handleAdminCalCancel();
void startAutoCalMode();
void cancelAutoCalMode();
void updateAutoCalMode();
void updateAutoCalBoardGraph();
void updateAutoCalLCD();  // v-pro: บรรทัดสถานะ LCD ระหว่างคาลิเบตอัตโนมัติจากเว็บ
int computeAutoCalHoldPhase();  // v-led: ตัดสินความนิ่งของค่าอ่านระหว่าง ACAL_MEASURING (0=ยังไม่นิ่ง/1=ใกล้นิ่ง/2=นิ่งแล้ว)
void updateAutoCalLED();        // v-led: ไฟสถานะ (Andon) ระหว่างคาลิเบตอัตโนมัติจากเว็บ ใช้ธรรมเนียมสีเดียวกับตรวจวัดปกติ
void enterMeasureAW();
void runMeasureAWTick();
void updateMeasureLEDs(int holdPhase, bool nearStable = false);  // v22: nearStable เพิ่มใหม่ (มี default กันโค้ดเดิมที่เรียกแบบพารามิเตอร์เดียวยังคอมไพล์ผ่าน)
bool systemHasFault();
void idleStatusLED(bool idleReady);
float calTableLookup(float raw);
float surfaceFactor(float aw, float tempC);      // v27: ตัวคูณจากพื้นผิวคาลิเบรต 2 มิติ (aw x อุณหภูมิ) — 1.0 เสมอถ้ายังไม่เคยบันทึกพื้นผิว
void loadCalSurface();
void handleAdminSurfaceCommit();
void handleAdminSurfaceClear();
void handleAdminSurfaceStatus();
void handleCalQuick();
void enterPredictAW();
void runPredictAWTick();
void computeEquilibriumPrediction();
void updatePredictLCD(float rawNow);
extern float graphEmaAw;  // v13: นิยามจริงอยู่ท้ายไฟล์ (ตัวกรอง EMA ของเส้นกราฟ) — โหมด Predict ใช้ค่านี้วาดกราฟ
bool fitAR1(const float* y, int n, float& vEq, float& tau);
bool fitAR2(const float* y, int n, float& vEq, float& tau);
void predClear();
float predEtaSec();
void predGraphReset();
void predGraphAccumulate(float v);
int pgY(float v);
void drawPredictLegend();
void drawPredictAxisLabels();
void drawPredictGraph(int holdPhase);
void drawPredictInfoTFT();
void drawIconPredict(int cx, int cy, int r);
void enterCompareAW();
void runCompareTick();
void updateCompareLCD(float v, int holdPhase, int idx);
void drawCompareResultScreen();
void drawIconCompare(int cx, int cy, int r);
int drawWifiQRCode(int x0, int y0, int maxSize);
void drawMenuDots(int count, int sel);
void drawButtonHint(const char* text);
void drawMenuScreen(const char* title, const char* const* items, const IconType* icons, int count, int sel);
void drawWifiInfoScreen();
void drawMascotFace(int cx, int cy, int r, bool eyesOpen);
void drawMascotCenter(const char* label);
void updateMascotIdleBlink();
void drawMascotSplashEntrance(int cx, int cy, int r);
void drawIconByType(IconType type, int cx, int cy, int r);
void drawCenterIcon(IconType type, const char* label);
void drawIconMeasure(int cx, int cy, int r);
void drawIconGear(int cx, int cy, int r);
void drawIconCross(int cx, int cy, int r);
void drawIconDrop(int cx, int cy, int r);
void drawIconCrystal(int cx, int cy, int r);
void drawIconPouch(int cx, int cy, int r);
void drawIconTrash(int cx, int cy, int r);
void drawIconWifi(int cx, int cy, int r);
void drawIconRecord(int cx, int cy, int r);
void triggerIcon(float atValue, int type);
int getFoodCategoryIndex(float v);
void clearFoodCategoryLabel();
void drawFoodCategory(int idx);
void drawElapsedTimeTFT(unsigned long startMs);
void drawFinalDurationTFT(unsigned long durationSec);
void updateLCD(float v, int holdPhase, int idx);
void pushValue(float v, float t);
void resetGraphFilter();
int stepSelIndex(int cur, int count, bool up, bool down);
void drawGraphLegend();
void drawGraphFrame();
void drawStaticUI();
void drawGraph(int holdPhase);
int valueToY(float v);
int valueToYTemp(float t);
void drawCurrentTemp(float t);
void drawCurrentValue(float v, int holdPhase);
void drawActiveIcon(int cx, int cy, unsigned long age);
void drawCookieIcon(int cx, int cy, int r);
void drawFruitIcon(int cx, int cy, int r);
void drawMeatIcon(int cx, int cy, int r);
void drawMilkIcon(int cx, int cy, int r);


bool updateButtonDebounce(BtnState& b, int pin, unsigned long now) {
  bool raw = digitalRead(pin);
  if (raw != b.lastRaw) {
    b.lastChangeTime = now;
    b.lastRaw = raw;
  }
  bool justReleased = false;
  if (now - b.lastChangeTime > DEBOUNCE_MS && b.stable != raw) {
    b.stable = raw;
    if (raw == LOW) {
      b.pressed = true;
      b.pressStart = now;
      b.longFired = false;
      b.chorded = false;
    } else {
      justReleased = b.pressed;
      b.pressed = false;
    }
  }
  return justReleased;
}

ButtonEvents pollButtons() {
  ButtonEvents e = { false, false, false, false };
  unsigned long now = millis();
  bool upReleased = updateButtonDebounce(btnUp, BTN_UP, now);
  bool downReleased = updateButtonDebounce(btnDown, BTN_DOWN, now);
  bool bothHeld = btnUp.pressed && btnDown.pressed;

  if (bothHeld) {
    btnUp.chorded = true;
    btnDown.chorded = true;
    unsigned long heldStart = (btnUp.pressStart > btnDown.pressStart) ? btnUp.pressStart : btnDown.pressStart;
    if (!chordLongFired && now - heldStart >= BTN_HOLD_MS) {
      e.exit = true;
      chordLongFired = true;
    }
  } else {
    chordLongFired = false;
    if (btnUp.pressed && !btnUp.chorded && !btnUp.longFired && now - btnUp.pressStart >= BTN_HOLD_MS) {
      e.select = true;
      btnUp.longFired = true;
    }
    if (btnDown.pressed && !btnDown.chorded && !btnDown.longFired && now - btnDown.pressStart >= BTN_HOLD_MS) {
      e.select = true;
      btnDown.longFired = true;
    }
  }

  if (upReleased && !btnUp.chorded && !btnUp.longFired) e.up = true;
  if (downReleased && !btnDown.chorded && !btnDown.longFired) e.down = true;

  if (upReleased) {
    btnUp.chorded = false;
    btnUp.longFired = false;
  }
  if (downReleased) {
    btnDown.chorded = false;
    btnDown.longFired = false;
  }
  return e;
}

// v12: เดิมไม่ว่ากดปุ่ม UP หรือ DOWN เมนูจะเลื่อนไปทิศทางเดียวกันเสมอ (selIndex+1 ทั้งคู่)
// ทำให้ปุ่ม UP ใช้งานไม่ได้จริง เปลี่ยนเป็นเลื่อน "ขึ้น" (index น้อยลง วนกลับไปท้ายสุดถ้าติดขอบบน)
// เมื่อกด UP และเลื่อน "ลง" (index มากขึ้น วนกลับไปต้นถ้าติดขอบล่าง) เมื่อกด DOWN ตามความหมายจริงของปุ่ม
int stepSelIndex(int cur, int count, bool up, bool down) {
  if (count <= 0) return 0;
  if (up) cur = (cur - 1 + count) % count;
  else if (down) cur = (cur + 1) % count;
  return cur;
}

// เขียนขา LED จริง ๆ ล้วน ๆ (ไม่บันทึกสถานะ) — setLED()/blinkLED() ด้านล่างเป็นตัวห่อที่บันทึกสถานะ "สีที่ตั้งใจ" ไว้ให้เว็บอ่าน
void setLEDPins(bool r, bool g, bool b) {
  digitalWrite(RED_PIN, r);
  digitalWrite(GREEN_PIN, g);
  digitalWrite(BLUE_PIN, b);
}

// v-led-web: สถานะไฟ LED ล่าสุดที่ "ตั้งใจ" จะแสดง (สีของ setLED()/blinkLED() ครั้งล่าสุดที่ถูกเรียก ไม่รวมจังหวะดับ
// ตอนกระพริบ) ให้เว็บแดชบอร์ดอ่านไปแสดงสีเดียวกับ LED จริงบนตัวเครื่องเป๊ะ ผ่านฟิลด์ ledColor/ledBlink ใน /data
bool ledRepR = false, ledRepG = false, ledRepB = false, ledRepBlink = false;

void setLED(bool r, bool g, bool b) {
  ledRepR = r; ledRepG = g; ledRepB = b; ledRepBlink = false;
  setLEDPins(r, g, b);
}
unsigned long blinkLast = 0;
bool blinkOn = true;
void blinkLED(bool r, bool g, bool b, unsigned long interval) {
  ledRepR = r; ledRepG = g; ledRepB = b; ledRepBlink = true;   // สีที่ตั้งใจไว้ (ไม่ใช่ค่าตอนดับ)
  if (millis() - blinkLast >= interval) {
    blinkLast = millis();
    blinkOn = !blinkOn;
  }
  if (blinkOn) setLEDPins(r, g, b);
  else setLEDPins(false, false, false);
}

// v-led-web: แปลงสถานะ r/g/b ล่าสุดเป็นชื่อสี (ไฟ RGB ของบอร์ดเป็นดิจิทัลล้วน on/off 3 เส้น จึงมีแค่ 8 สีเป็นไปได้)
// ให้ตรงกับผังสีที่ใช้จริงทั้งเครื่อง (ดูคำอธิบายผังสีด้านบน): off/red/green/blue/yellow/purple/cyan/white
const char* ledColorName() {
  if (!ledRepR && !ledRepG && !ledRepB) return "off";
  if (ledRepR && !ledRepG && !ledRepB) return "red";
  if (!ledRepR && ledRepG && !ledRepB) return "green";
  if (!ledRepR && !ledRepG && ledRepB) return "blue";
  if (ledRepR && ledRepG && !ledRepB) return "yellow";
  if (ledRepR && !ledRepG && ledRepB) return "purple";
  if (!ledRepR && ledRepG && ledRepB) return "cyan";
  return "white";
}

// ============================================================================
//  v22: ไฟ LED แจ้งสถานะละเอียดขึ้น — ไฟ RGB ของบอร์ดเป็นแบบดิจิทัลล้วน (on/off 3 เส้น) ไม่มี PWM ผสมสี
//  จึงทำสีได้จริงแค่ 8 แบบ: ปิด / แดง / เขียว / น้ำเงิน / เหลือง(R+G) / ม่วง-แดงอมน้ำเงิน(R+B) / ฟ้าอมเขียว-cyan(G+B) / ขาว
//  (ทำ "ส้ม" ตรง ๆ ไม่ได้เพราะไม่มี PWM ผสมสัดส่วน R/G — ใช้ "เขียวกระพริบเร็ว" แทนความหมาย "ใกล้นิ่งมากแล้ว" แทน)
//  ผังสีใหม่ทั้งเครื่อง:
//    แดงนิ่ง       = ค่ายังเคลื่อนที่ไม่นิ่ง (เดิม)
//    เหลืองนิ่ง     = ค่าคงที่ระดับหนึ่งแล้ว กำลังรอครบหน้าต่างนิ่ง (เดิม)
//    เขียวกระพริบเร็ว = "ใกล้นิ่งมาก" ช่วงกว้างค่าแคบพอแล้ว แค่รอเวลา/ความชันให้ครบเกณฑ์ล็อก (ใหม่ v22)
//    เขียวนิ่ง      = นิ่งสนิท ล็อกค่าได้ (เดิม)
//    น้ำเงินนิ่ง     = กำลังฮีตเซนเซอร์ SHT ก่อน/หลังวัด (v22: ย้ายมาจากม่วง กันชนกับความหมายใหม่ของม่วง)
//    ฟ้าอมเขียว(cyan)= กำลังรอเซนเซอร์เย็นลงหลังฮีต / หรือเครื่องว่างอยู่แต่ "พร้อมทำงาน" (เช่น หน้าดูบันทึกเดิม)
//    ม่วงกระพริบเร็ว  = FAULT ของระบบ (เซนเซอร์ความชื้น/อุณหภูมิหลุด หรือ Wi-Fi AP เปิดไม่สำเร็จ) — สงวนไว้เฉพาะ fault เท่านั้น
// ============================================================================

// รวม flag fault ที่ "ต้องแก้ก่อนวัดต่อ" ไว้จุดเดียว ใช้ตัดสินไฟม่วงกระพริบ (ไม่รวม peltierStuckHot/coldRoomWarn
// เพราะสองอันนั้นยังมีไฟแดง/ข้อความเตือนของตัวเองอยู่แล้วในหน้าที่เกี่ยวข้อง เป็นเรื่อง "ห้องยังไม่ถึงอุณหภูมิ" ไม่ใช่ฮาร์ดแวร์เสีย)
bool systemHasFault() {
  return sensorFaultSHT || sensorFaultDS18B20 || wifiApFault;
}

// ไฟ LED สำหรับหน้าจอที่ไม่ได้กำลังวัด/คาลิเบตอัตโนมัติอยู่ (เมนูต่าง ๆ) — เรียกแทน setLED(...) ตรง ๆ ที่จุดเหล่านั้น
// เพื่อให้ "มี fault" ชนะไฟ idle ปกติเสมอ ผู้ใช้เห็นทันทีว่าต้องเช็คเครื่องก่อนเริ่มวัดครั้งถัดไป
//   idleReady = false -> ปิดไฟ (เมนูทั่วไป, ค่าเดิม) / true -> ฟ้าอมเขียว (cyan, หน้าดูบันทึกเดิม บอกว่าเครื่องพร้อมทำงานอยู่)
void idleStatusLED(bool idleReady) {
  if (systemHasFault()) { blinkLED(true, false, true, 300); return; }  // ม่วงกระพริบเร็ว = fault
  setLED(false, idleReady, idleReady);  // false=ปิด / true=cyan (G+B)
}

// ============================================================================
//  ระบบคาลิเบรต Aw v6.3 — ตารางจุดคาลิเบรตหลายจุด + เส้นตรงต่อกันเป็นช่วง ๆ (piecewise-linear)
//  แต่ละช่วงระหว่างจุดจะถูก "ยืด" หรือ "หด" สเกลอิสระจากกัน ต่างจากสมการเส้นตรงเส้นเดียวแบบเดิม
// ============================================================================

// ตารางจุดคาลิเบรต: {raw RH เศษส่วน 0-1 ตามที่ readRawAw() คืนค่า , aw อ้างอิงที่ต้องการ}
// เรียงลำดับ raw จากน้อยไปมาก (ต้องเรียงเสมอ ไม่งั้นการค้นหาช่วงจะผิดพลาด)
//   raw 0.00  -> aw 0.00    (จุดกำเนิด: RH ดิบ 0% ควรให้ aw ใกล้ 0 ตามหลักฟิสิกส์)
//   raw 0.325 -> aw 0.22    (เดิมอยู่ที่ raw 0.55 แต่ย้ายมาไว้ตรงนี้แทน เพราะ raw 0.5-0.6 ตรงกับ
//                            "ความชื้นห้องปกติ" ซึ่งควรให้ aw ออกมาใกล้เคียง 0.5-0.6 ไม่ใช่ถูกบีบให้ต่ำ
//                            (เลือก 0.325 = ค่ากึ่งกลางของช่วง 0.3-0.35 ที่ต้องการ))
//   raw 0.70  -> aw 0.753
//   raw 0.98  -> aw 1.00
// ผลจากการย้ายจุด: ช่วง raw 0.325-0.70 มีความชัน (0.753-0.22)/(0.70-0.325) = 1.4213
// ทำให้ raw 0.55 -> aw ~0.540 และ raw 0.60 -> aw ~0.611 (ใกล้เคียง identity ในช่วงความชื้นห้องปกติแล้ว)
// วิธีเพิ่ม/แก้จุดคาลิเบรต: v-accuracy — ไม่ต้องแก้ตรงนี้แล้วอัปโหลดเฟิร์มแวร์ใหม่ทุกครั้งอีกต่อไป
// แก้ได้จากหน้าเว็บแดชบอร์ด (แผง "Calibration Points") ซึ่งจะบันทึกลง NVS (Preferences) ถาวร
// ค่าด้านล่างนี้ ("CAL_POINTS_FACTORY") เป็นแค่ค่าเริ่มต้น/ค่าโรงงานที่ใช้ตอนยังไม่เคยบันทึกผ่านเว็บเลย
// หรือตอนกด "รีเซ็ตเป็นค่าโรงงาน" — ดู loadCalPoints()/saveCalPoints() และ handleCalGet()/handleCalSet() ท้ายไฟล์
// (struct CalPoint นิยามไว้ก่อนหน้านี้แล้ว ใกล้ส่วน Forward declarations เพราะ prototype ต้องใช้ชนิดนี้)
// v-cal-reset: ผู้ใช้ขอ "คาลิเบตใหม่หมด" — ล้างตารางชั่วคราว/provisional ชุดเดิม (ที่ fit จากข้อมูลไม่กี่จุด
// ตอนเกณฑ์ "นิ่ง" ยังสั้นแค่ ~15-22 วิ และยังไม่ได้ตั้ง SHT_MINUS_DS_OFFSET_C จริง) ทิ้งทั้งหมด
// ตารางโรงงานด้านล่างนี้จึงกลับไปเป็น "identity" (raw = aw ตรง ๆ ไม่ยืด/บีบสเกลเลย) คือค่ากลาง ๆ ที่ไม่ฝืนอคติ
// ไปทางไหน ใช้เป็นจุดเริ่มต้นตอนยังไม่มีข้อมูลจริงเท่านั้น — ของจริงให้ใช้ปุ่ม "เริ่มคาลิเบตอัตโนมัติ" ในแผงแอดมิน
// บนเว็บ (วัด 5 รอบ: รอบ 1-3 ที่ 25°C, รอบ 4 ที่ 20°C, รอบ 5 ที่ 17°C — เก็บทั้งค่าดิบ/RAW เฉลี่ยและค่า aw
// คาลิเบรตแล้วเฉลี่ยของแต่ละรอบไว้ในตาราง "โหมดคาลิเบตอัตโนมัติ" ให้เห็นครบ) แล้วกดบันทึกจุดคาลิเบรตจริงจากผลที่ได้อีกที
// หมายเหตุ: ถ้าเคยกด "บันทึกจุดคาลิเบรต" ผ่านเว็บไว้ก่อนหน้านี้ ค่าที่บันทึกจะยังค้างอยู่ใน NVS ของบอร์ด (ตารางนี้
// เป็นแค่ค่า fallback ตอนยังไม่เคยบันทึกเลย) กดปุ่ม "รีเซ็ตเป็นค่าโรงงาน" ในแผงคาลิเบรตบนเว็บอีกครั้งเพื่อล้าง
// ค่าเก่าใน NVS ให้กลับมาเป็น identity ชุดนี้จริง ๆ ก่อนเริ่มคาลิเบตอัตโนมัติรอบใหม่
// v-multi-sample: เพิ่มจาก 4 เป็น 6 จุด เพื่อรองรับการคาลิเบตด้วยสารละลายมาตรฐานหลายตัวอย่างขึ้น (เส้นโค้งคาลิเบรตแม่นขึ้น
// ทั้งช่วง 0-1) — ถ้าแก้ตัวเลขนี้ ต้องแก้ค่า CAL_POINTS_TOTAL ในหน้าเว็บ (ค้นหา "CAL_POINTS_TOTAL" ในไฟล์นี้) ให้ตรงกันด้วย
// v15: เพิ่มจาก 6 เป็น 9 จุด (0,0 + 8 ระดับ) — แต่ละช่วงระหว่างจุดจึงแคบลง ทำให้เส้น piecewise-linear
// ตามความโค้งจริงของเซนเซอร์ได้ละเอียดขึ้น โดยเฉพาะช่วง RH ต่ำ (<0.2) และช่วงสูงใกล้อิ่มตัว (>0.9) ที่เซนเซอร์
// capacitive มักไม่เชิงเส้นมากกว่าช่วงกลาง — รองรับสารละลายมาตรฐานอ้างอิงได้ถึง 8 ระดับ เช่น
// LiCl 0.113 · CH3COOK 0.225 · MgCl2 0.328 · K2CO3 0.432 · NaBr 0.577 · NaCl 0.753 · KCl 0.843 · K2SO4 0.973 (ที่ 25°C)
// v-est-2026-09-25: ตารางประมาณการชั่วคราว (INTERIM / BEST-GUESS) — แทนค่า identity เดิม
// ที่มา: เทียบผล 3 การวัดจริงจากกราฟ AWG-CMP-20260925 (MgCl2 / NaCl / KCl) กับค่าอ้างอิงมาตรฐาน
//   - MgCl2 : raw(mean)=0.491  ->  ref aw=0.328   [MEASURED, เชื่อถือได้ — ใช้เป็น anchor]
//   - NaCl  : raw(mean)=0.611  ->  ref aw=0.753   [MEASURED, เชื่อถือได้ — ใช้เป็น anchor]
//   - KCl   : raw(mean)=0.580  ->  ref aw=0.830   [MEASURED แต่ "ใช้ไม่ได้ตรง ๆ" — raw ต่ำกว่า NaCl
//             ทั้งที่ aw จริงสูงกว่า (ไม่ monotonic, isCalPointsMonotonic() จะปฏิเสธ) สาเหตุที่น่าจะเป็นไปได้มากที่สุด
//             คือกราฟ KCl ยังไม่นิ่งสมดุลจริงตอนบันทึก (ดูกราฟ AWG-CMP-20260925-R3B02 เส้นยังไล่ขึ้นช้า ๆ
//             ตลอด 25 นาที) — จุด KCl ด้านล่างจึง "ประมาณการต่อแนวโน้ม" จากช่วง MgCl2->NaCl แทนค่าที่วัดได้จริง
//             ต้องคาลิเบตซ้ำ (ยืดเวลาการวัดให้นิ่งจริงก่อน) แล้วแทนที่ค่านี้ด้วยค่าจริงภายหลัง
// จุดที่เหลือ (LiCl, CH3COOK, K2CO3, NaBr, K2SO4) ไม่มีข้อมูลวัดจริง -> ประมาณด้วยการเทียบสัดส่วนเชิงเส้น (lerp)
// จากช่วงที่ใกล้ที่สุดที่มี anchor จริง ถือเป็นค่า "เดาแบบมีหลักการ" ไว้นำเสนอ/เริ่มต้นเท่านั้น ไม่ใช่ค่าคาลิเบรตจริง
// TODO: แทนที่บรรทัดที่คอมเมนต์ "ESTIMATED" ด้วยค่าจริงทันทีที่มีผลคาลิเบตอัตโนมัติ/Quick Cal จริงของแต่ละจุด
// v-cal-2026-09-29b: ตารางแก้ไขหลังทดสอบจริง — ตารางรอบแรกของวันนี้ (สโลป 6.3 ระหว่าง CaCl2 กับน้ำประปา) ทำให้ห้องเปล่า
//   (RH 56.9% = raw 0.569) อ่านได้ aw ~0.78 เพราะกราฟ 12 นาทีนั้นแต่ละช่วงสั้นแค่ 1.5-3 นาที ค่ายังไม่สมดุล จึงใช้เป็น anchor ไม่ได้
//   ตารางนี้ยึดข้อมูลที่ "นิ่งสมดุลจริง" (MgCl2 / NaCl จากการวัด 25 นาที ของ 09-25) + จุดห้องเปล่า (เซนเซอร์อ่าน RH เอง ไม่มีตัวอย่าง)
//   CaCl2 (aw 0.29) ที่ raw 0.486-0.495 ตารางนี้อ่านได้ ~0.33-0.34 (คลาด +0.04-0.05 ยอมรับได้ ยังไม่สมดุล)
//   จุด ESTIMATED = ยังไม่ได้วัดจริง — น้ำประปา/NaCl/KCl ต้องวัดให้นิ่ง (ไฟเขียว) อย่างน้อย 10-25 นาทีก่อนแก้จุดบน
const int CAL_POINTS_COUNT = 27;
// ตารางเริ่มต้นแบบ identity เท่านั้น — ไม่ใช้ค่าประมาณของสาร/ห้องเป็นคาลิเบรต
// ผู้ใช้ต้องเก็บค่ามาตรฐานจริง 27 ระดับ แล้วบันทึกผ่านเว็บก่อนใช้เป็นผลเชิงปริมาณ
const CalPoint CAL_POINTS_FACTORY[CAL_POINTS_COUNT] = {
  { 0.000000f, 0.000000f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.038462f, 0.038462f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.076923f, 0.076923f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.115385f, 0.115385f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.153846f, 0.153846f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.192308f, 0.192308f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.230769f, 0.230769f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.269231f, 0.269231f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.307692f, 0.307692f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.346154f, 0.346154f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.384615f, 0.384615f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.423077f, 0.423077f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.461538f, 0.461538f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.500000f, 0.500000f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.538462f, 0.538462f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.576923f, 0.576923f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.615385f, 0.615385f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.653846f, 0.653846f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.692308f, 0.692308f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.730769f, 0.730769f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.769231f, 0.769231f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.807692f, 0.807692f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.846154f, 0.846154f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.884615f, 0.884615f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.923077f, 0.923077f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 0.961538f, 0.961538f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
  { 1.000000f, 1.000000f },  // จุดเริ่มต้นปลอดภัย; ต้องแทนด้วยค่ามาตรฐานที่วัดจริง
};
// v-accuracy: จุดคาลิเบรตที่ใช้งานจริงขณะรันไทม์ — ไม่ใช่ const คงที่เหมือนเดิม โหลดจาก NVS ตอนบูตผ่าน
// loadCalPoints() (เรียกใน setup()) ถ้ายังไม่เคยบันทึกไว้ หรือข้อมูลที่โหลดมาผิดปกติ จะ fallback ไปใช้ค่าโรงงานข้างบน
CalPoint calPoints[CAL_POINTS_COUNT];

// ช่วงอุณหภูมิที่สมการถดถอย (regression) นี้ถูก fit ไว้จริง — ควรตั้งให้ตรงกับช่วงอุณหภูมิที่ใช้เก็บ
// ข้อมูลตอนหาค่า C0..C4 จริง (ปรับตัวเลขสองบรรทัดนี้ให้ตรงกับข้อมูลจริงของคุณ)
// เหตุผลที่ต้อง "clamp" ค่าอุณหภูมิก่อนป้อนเข้าสมการ: ถ้าห้องร้อน/เย็นเกินกว่าที่เทลเทียร์จะไล่ทัน (เช่น
// วันที่อากาศร้อนมาก เทลเทียร์เปิดเต็มกำลังตลอดแต่ก็ยังกดอุณหภูมิลงไม่ถึงเป้าหมาย) อุณหภูมิจริงอาจหลุดออกไป
// นอกช่วงที่เคย fit ไว้มาก ๆ ซึ่งเทอมกำลังสอง (C3*RH² และโดยเฉพาะ C4*T²) จะทำให้ค่าที่คำนวณได้เบี่ยงเบน
// ("extrapolate ไปไกลเกิน") รุนแรงกว่าความเป็นจริงมาก การ clamp ไว้ที่ขอบเขตนี้ช่วยกันไม่ให้ error ยิ่งแย่ลง
// ไปกว่าที่ขอบเขตการ fit เดิมเคยรับประกันความแม่นยำไว้ (ดีกว่าปล่อยให้สมการยิงค่าออกนอกลู่นอกทางไปเรื่อย ๆ)
const float CAL_TEMP_MIN_C = 20.0;
const float CAL_TEMP_MAX_C = 30.0;

// v11: ข้อมูลอุปกรณ์/เซนเซอร์/สมการคาลิเบรต ที่ต้องมีติดไปกับไฟล์ข้อมูลที่ส่งออกทุกครั้ง เพื่อให้ผลวัด
// ตรวจสอบย้อนกลับได้ (traceability) ตามหลักที่มาตรฐานการวัดทั่วไป (เช่น ISO 18787 สำหรับ water activity
// หรือ มผช. ที่อ้างอิงวิธีทดสอบมาตรฐาน) มักต้องการ — ได้แก่ รุ่นเฟิร์มแวร์, ชนิดเซนเซอร์, อุณหภูมิเป้าหมาย,
// ช่วงอุณหภูมิที่คาลิเบรตไว้จริง, เกณฑ์ความนิ่งที่ใช้ตัดสิน, และจุดคาลิเบรตที่ใช้แปลงค่า ณ ขณะนั้น
// หมายเหตุ: นี่คือการแนบข้อมูลเมทาดาทาให้ครบถ้วนพอสำหรับตรวจสอบย้อนกลับเท่านั้น ไม่ใช่ใบรับรองมาตรฐาน —
// การขอรับรองตาม ISO/มผช. จริงต้องผ่านกระบวนการสอบเทียบ/ตรวจประเมินโดยหน่วยงานที่ได้รับการรับรองแยกต่างหาก
#if AW_RAW_MODE
const char* FW_VERSION = "AW-Meter Firmware v18 RAW (PWM-aware stability + fast block stability + predict + temp compensation, uncalibrated aw)";
const char* VALUE_MODE = "RAW";
#else
const char* FW_VERSION = "AW-Meter Firmware v18 (PWM-aware stability + 10-round auto-cal + fast block stability + unified LED + finer calibration + refined predict + temp compensation)";
const char* VALUE_MODE = "CALIBRATED";
#endif
// v17: เดิมเป็น const char* ค่าเดียวตายตัว — เปลี่ยนเป็นฟังก์ชันคืนชื่อรุ่นตามเซนเซอร์ที่ detectHumiditySensor()
// ตรวจพบจริงตอนบูต (SHT45 ผ่าน I2C หรือ DHT22/11 ผ่านขา DHT_PIN) ไปโผล่ในฟิลด์ JSON "humiditySensor" อัตโนมัติ
// (เว็บแดชบอร์ดหน้า System Health + ใบรายงานผล COA อ่านฟิลด์นี้อยู่แล้ว ไม่ต้องแก้ฝั่งเว็บเพิ่ม)
const char* sensorModelHumidity() {
  if (humSensorType == HUMSENS_DHT) {
#if DHT_TYPE == DHT22
    return "DHT22 (single-wire RH/T sensor, GPIO15)";
#else
    return "DHT11 (single-wire RH/T sensor, GPIO15)";
#endif
  }
  if (humSensorType == HUMSENS_SHT) return "Sensirion SHT45 (I2C capacitive RH sensor)";
  return "No humidity sensor detected (checked SHT I2C 0x44/0x45 and DHT GPIO15)";
}
const char* SENSOR_MODEL_TEMP = "DS18B20 (1-Wire digital temperature sensor)";

// คืนค่า true ถ้าอุณหภูมิปัจจุบันหลุดช่วงที่คาลิเบรตไว้ (ใช้เตือนผู้ใช้บนจอว่าค่าที่วัดได้อาจแม่นยำน้อยลง
// แม้เทลเทียร์จะพยายามควบคุมอุณหภูมิเต็มที่แล้วก็ตาม เช่น ห้องร้อน/เย็นเกินกำลังของเทลเทียร์)
bool isTempOutOfCalRange(float t) {
  if (isnan(t)) return false;
  return (t < CAL_TEMP_MIN_C) || (t > CAL_TEMP_MAX_C);
}

// แปลงค่าดิบ (raw = เศษส่วน RH 0-1 ตามที่ readRawAw() คืนค่ามาโดยไม่เปลี่ยนหน่วยเดิม) เป็นค่า aw
// ที่คาลิเบรตแล้ว โดยหาตำแหน่ง raw ว่าอยู่ระหว่างจุดคู่ไหนในตาราง CAL_POINTS แล้วเทียบสัดส่วนเชิงเส้น
// (piecewise-linear interpolation) ระหว่างจุดสองจุดนั้น — แต่ละช่วงจึงยืด/หดสเกลได้อิสระจากช่วงอื่น
// ถ้า raw อยู่นอกช่วงต่ำสุด/สูงสุดของตาราง จะต่อเส้นตรงจากช่วงปลายสุดออกไป (extrapolate) แล้วจึง
// constrain ผลลัพธ์ให้อยู่ใน 0.0-1.0 เสมอ (พารามิเตอร์ tempC เก็บไว้เผื่อขยายเป็นสมการที่ขึ้นกับอุณหภูมิในอนาคต)
// v22: แยกส่วน piecewise-linear lookup ออกจาก applyCal() เป็นฟังก์ชันของตัวเอง (ค่าที่ได้ "ก่อน" คูณตัวชดเชยอุณหภูมิ)
// เพื่อให้ handleCalQuick() (คาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก) คำนวณค่าที่ตารางควรทำนายได้ตรง ๆ โดยไม่ต้องแยก
// ตัวคูณ gradientFactor() ออกเองซ้ำที่อื่น — ตรรกะเดิมของ applyCal() ไม่เปลี่ยนแปลง แค่ย้ายมาไว้ในฟังก์ชันแยก
// v-cal-priority: ลำดับ 3) ของ 3 ลำดับความสำคัญการคาลิเบต (ดูคำอธิบายเต็มที่ applyCal()) — ฐานล่างสุดที่ทุกอย่าง
// ยืนอยู่บน ตารางนี้รวมผลของลำดับ 1 (Quick Cal) ไว้แล้วเพราะ handleCalQuick() แก้ calPoints ตรงนี้โดยตรง
// ---------- v26 (ชุด C): การเก็บมุมหักของตารางคาลิเบรตด้วย PCHIP แบบ "จำกัดการเบี่ยงเบน" ----------
// ตารางคาลิเบรต (CAL_POINTS_FACTORY / ค่าใน NVS / ค่า Quick Cal) ไม่ถูกแตะเลย — ทุกจุดโหนดให้ค่า aw เท่าเดิมเป๊ะ
// และช่วงที่ตารางเป็นเส้นตรงอยู่แล้ว (เช่น 0 -> MgCl2 ที่จุดประมาณทั้งหมดอยู่บนเส้นเดียวกัน) ก็ได้ค่าเท่าเดิมเป๊ะ
// เปลี่ยนเฉพาะ "ระหว่างโหนดตรงมุมหัก" จากเส้นตรงเป็นเส้นโค้ง PCHIP (Fritsch-Carlson: เอกทิศ ไม่ overshoot ไม่เกิดคลื่น)
// โดยบังคับไม่ให้ค่าที่ได้ต่างจากเส้นตรงเดิมเกิน CAL_PCHIP_MAX_DEV_AW (aw) — ตั้งเป็น 0 = พฤติกรรมเดิมทุกประการ (เส้นตรงล้วน)
// การจำกัด: เบี่ยงเบนสูงสุดของ Hermite จากเส้นตรง <= h x (4/27) x (|d0-s| + |d1-s|) จึงจำกัด |d-s| <= 27 x eps / (8 h) ต่อปลายช่วง
// (s = ความชันเส้นตรงของช่วงนั้น, d = ความชันที่โหนด) ค่านอกช่วงตาราง (extrapolation) ยังเป็นเส้นตรงเดิมทุกประการ
const float CAL_PCHIP_MAX_DEV_AW = 0.001f;   // เบี่ยงเบนสูงสุดจากเส้นตรงเดิม (aw) — 0 = ปิด PCHIP, ต่ำกว่าเกณฑ์ทดสอบ +-0.0015

static float calSecant(int i) {
  return (calPoints[i + 1].aw - calPoints[i].aw) / (calPoints[i + 1].raw - calPoints[i].raw);
}
// ความชันที่โหนด k ตามสูตร PCHIP (ค่าเฉลี่ยฮาร์มอนิกถ่วงน้ำหนักตามความกว้างช่วง; ปลายตารางใช้สูตรสามจุดแบบรักษารูปร่าง)
static float calNodeDeriv(int k) {
  const int N = CAL_POINTS_COUNT;
  if (k == 0) {
    float h0 = calPoints[1].raw - calPoints[0].raw, h1 = calPoints[2].raw - calPoints[1].raw;
    float s0 = calSecant(0), s1 = calSecant(1);
    float d = ((2.0f * h0 + h1) * s0 - h0 * s1) / (h0 + h1);
    if (d * s0 <= 0.0f) d = 0.0f;
    else if (s0 * s1 <= 0.0f && fabsf(d) > 3.0f * fabsf(s0)) d = 3.0f * s0;
    return d;
  }
  if (k == N - 1) {
    float h1 = calPoints[N - 1].raw - calPoints[N - 2].raw, h0 = calPoints[N - 2].raw - calPoints[N - 3].raw;
    float s1 = calSecant(N - 2), s0 = calSecant(N - 3);
    float d = ((2.0f * h1 + h0) * s1 - h1 * s0) / (h1 + h0);
    if (d * s1 <= 0.0f) d = 0.0f;
    else if (s1 * s0 <= 0.0f && fabsf(d) > 3.0f * fabsf(s1)) d = 3.0f * s1;
    return d;
  }
  float sl = calSecant(k - 1), sr = calSecant(k);
  if (sl * sr <= 0.0f) return 0.0f;                       // จุดกลับทิศ/ราบ -> ความชัน 0 (ไม่ overshoot)
  float hl = calPoints[k].raw - calPoints[k - 1].raw, hr = calPoints[k + 1].raw - calPoints[k].raw;
  float w1 = 2.0f * hr + hl, w2 = hr + 2.0f * hl;
  return (w1 + w2) / (w1 / sl + w2 / sr);
}

float calTableLookup(float raw) {
  float aw;
  if (raw <= calPoints[0].raw) {
    // ต่ำกว่าจุดแรก -> ต่อเส้นตรงจากความชันของช่วงแรกออกไป
    float slope = (calPoints[1].aw - calPoints[0].aw) / (calPoints[1].raw - calPoints[0].raw);
    aw = calPoints[0].aw + slope * (raw - calPoints[0].raw);
  } else if (raw >= calPoints[CAL_POINTS_COUNT - 1].raw) {
    // สูงกว่าจุดสุดท้าย -> ต่อเส้นตรงจากความชันของช่วงสุดท้ายออกไป
    float slope = (calPoints[CAL_POINTS_COUNT - 1].aw - calPoints[CAL_POINTS_COUNT - 2].aw)
                / (calPoints[CAL_POINTS_COUNT - 1].raw - calPoints[CAL_POINTS_COUNT - 2].raw);
    aw = calPoints[CAL_POINTS_COUNT - 1].aw + slope * (raw - calPoints[CAL_POINTS_COUNT - 1].raw);
  } else {
    // อยู่ระหว่างจุดสองจุดในตาราง -> หาช่วงที่ raw ตกอยู่
    aw = calPoints[CAL_POINTS_COUNT - 1].aw;  // ค่าเริ่มต้นกันกรณีไม่เข้าเงื่อนไขใดเลย (ไม่ควรเกิดขึ้น)
    for (int i = 0; i < CAL_POINTS_COUNT - 1; i++) {
      if (raw >= calPoints[i].raw && raw <= calPoints[i + 1].raw) {
        float h = calPoints[i + 1].raw - calPoints[i].raw;
        float t = (raw - calPoints[i].raw) / h;
        float y0 = calPoints[i].aw, y1 = calPoints[i + 1].aw;
        aw = y0 + t * (y1 - y0);                     // เส้นตรงเดิม (ใช้เมื่อปิด PCHIP)
        if (CAL_PCHIP_MAX_DEV_AW > 0.0f && CAL_POINTS_COUNT >= 3 && h > 1e-6f) {
          float sec = (y1 - y0) / h;
          float m = 3.375f * CAL_PCHIP_MAX_DEV_AW / h;   // 27 eps / (8 h)
          float d0 = constrain(calNodeDeriv(i), sec - m, sec + m);
          float d1 = constrain(calNodeDeriv(i + 1), sec - m, sec + m);
          float t2 = t * t, t3 = t2 * t;
          float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f, h10 = t3 - 2.0f * t2 + t;
          float h01 = -2.0f * t3 + 3.0f * t2,       h11 = t3 - t2;
          float v = h00 * y0 + h10 * h * d0 + h01 * y1 + h11 * h * d1;
          aw = constrain(v, fminf(y0, y1), fmaxf(y0, y1));   // กันหลุดช่วงระหว่างสองโหนดเสมอ
        }
        break;
      }
    }
  }
  return aw;
}

// ============================================================================
//  ลำดับความสำคัญของการคาลิเบต (แนวคิด — ไม่ได้เปลี่ยนพฤติกรรมการคำนวณจริง แค่ทำให้เห็นลำดับชัดเจนขึ้น):
//    ลำดับ 1) "ใส่ค่าจากเครื่องสอบเทียบ" (Quick Cal, ดู handleCalQuick()) — ค่าที่แม่นที่สุด เพราะมาจากการเทียบกับ
//             เครื่องอ้างอิงภายนอกจริง ณ ตัวอย่างเดียวกัน จึงมีสิทธิ์ "แก้ไขตารางคาลิเบรต" (calPoints) ได้โดยตรง
//             ผลของ Quick Cal จึงถาวรและเป็นรากฐานให้ลำดับ 3) ใช้ต่อ (ไม่ใช่ค่าที่มาแทนที่ผลลัพธ์แบบสดชั่วคราว)
//    ลำดับ 2) "เกณฑ์ที่ชดเชยอุณหภูมิแล้ว" (gradientFactor(), สูตร Magnus) — ปรับผลจากลำดับ 3) อีกชั้นหนึ่งตามส่วนต่าง
//             อุณหภูมิชิป SHT กับตัวอย่าง ณ ขณะนั้น ให้ตรง "ชนิดอุณหภูมิ" ที่วัด อยู่เหนือลำดับ 3) เพราะเป็นการแก้ไข
//             คลาดเคลื่อนที่เกิดขึ้น "เฉพาะขณะนั้น" (เปลี่ยนไปตามอุณหภูมิห้อง/ตัวอย่างสด ๆ ไม่ได้ถูกบันทึกลงตาราง)
//    ลำดับ 3) "คาลิเบตปกติ" (calTableLookup(), ตาราง piecewise-linear CAL_POINTS) — ฐานล่างสุดที่ทุกอย่างยืนอยู่บน
//             เป็นค่าที่ใช้เมื่อไม่มีการชดเชยอุณหภูมิ (ข้อมูลเซนเซอร์ไม่ครบ -> gradientFactor() คืน 1.0 อัตโนมัติ)
//    ผลจริงที่คำนวณคือ aw = calTableLookup(raw) [ลำดับ 3, รวมผล Quick Cal ของลำดับ 1 ที่แก้ตารางไว้ก่อนหน้าแล้ว]
//    คูณด้วย gradientFactor() [ลำดับ 2] เสมอ — ลำดับ 1-2-3 ข้างต้นคือลำดับ "ความน่าเชื่อถือ/ที่มา" ของแต่ละชั้น
//    ไม่ใช่ลำดับการ if-else เลือกอย่างใดอย่างหนึ่ง เพราะทั้งสามชั้นทำงานร่วมกันเสมอ
// ============================================================================
float applyCal(float raw, float tempC) {
#if AW_RAW_MODE
  // RAW: ไม่คาลิเบรต — คืนค่า RH ดิบ (เศษส่วน 0-1) ตรง ๆ แค่กันค่าหลุดช่วง
  (void)tempC;
  return constrain(raw, 0.0, 1.0);
#else
  if (isnan(tempC)) tempC = 25.0;  // ไม่รู้อุณหภูมิ -> อิงค่าที่ 25°C เป็นค่าเริ่มต้นปลอดภัย (ยังไม่ได้ใช้ในสูตรปัจจุบัน)
  // ลำดับ 3) คาลิเบตปกติ: ตารางนี้รวมผลของลำดับ 1 (Quick Cal) ไว้อยู่แล้ว เพราะ handleCalQuick() แก้ calPoints ตรงนี้โดยตรง
  float aw = calTableLookup(raw);
  // v14: ชดเชยความต่างอุณหภูมิระหว่างตัวชิป SHT กับตัวอย่าง (ทำหลังตารางคาลิเบรต: ตารางแก้ความคลาดของเซนเซอร์เอง,
  // ตัวคูณนี้แปลง RH ที่อ้างอิงอุณหภูมิชิป -> aw ที่อ้างอิงอุณหภูมิตัวอย่าง)
  // ลำดับ 2) เกณฑ์ชดเชยอุณหภูมิ: คูณทับผลลัพธ์ของลำดับ 3 เสมอ (คืน 1.0 เฉย ๆ ถ้าข้อมูลเซนเซอร์ไม่พอ = ไม่มีผลอะไรเพิ่ม)
  aw *= gradientFactor();
  aw *= surfaceFactor(aw, tempC);   // v27: พื้นผิว aw x อุณหภูมิ (=1.0 ถ้ายังไม่เคยบันทึก -> ผลเท่าเดิมเป๊ะ)
  return constrain(aw, 0.0, 1.0);
#endif
}

// ============================================================================
//  v-trend-offset (2026-09-28, v2 2026-09-29): ปรับ offset ของค่า aw ตาม "แนวโน้มของค่าดิบ (raw)" แบบต่อเนื่อง
//  แนวคิด: ตารางคาลิเบรตช่วง raw 0.49-0.61 ระหว่างจุดวัดจริง (MgCl2 -> NaCl) เป็นค่า "ประมาณ" จึงใช้แนวโน้มของ raw ตัดสินว่า
//  ตัวอย่างน่าจะอยู่ในโซนไหน แล้วขยับค่า aw ไปทางเส้นของโซนนั้นเท่านั้น (ไม่กระโดด ไม่แก้ตารางที่บันทึกใน NVS):
//    โซน 0 (ต่ำ/นิ่ง/ลดลง — เช่น ขนมกรอบ, มาม่า)     : เส้นผ่านจุดกำเนิดกับ anchor MgCl2 ; ปรับได้ไม่เกิน +/-0.10 จากค่าตาราง
//    โซน 1 (กำลังขึ้นปานกลาง)                        : เส้นต่อ MgCl2->NaCl ; เพดาน aw = 0.75
//    โซน 2 (ขึ้นเร็วต่อเนื่อง)                          : เส้นต่อ NaCl->KCl   ; เพดาน aw = 0.90
//    โซน 3 (ขึ้นเร็วมาก/ยังไม่หยุด — เช่น น้ำ, วุ้นเส้น) : เส้นต่อ KCl->น้ำบริสุทธิ์(1,1) ; เพดาน aw = 0.99
//  v2: เพิ่ม 2 เทคนิคให้ "เดา" แม่น/ไวขึ้น (ตามที่ขอ):
//   (ก) ขยายช่วง raw ที่ระบบยอมปรับ (TO_WIN_*) ให้กว้างขึ้นทั้งด้านต่ำ (ของแห้ง) และด้านสูง (ของเปียกจัด/น้ำ) แทนที่จะ
//       จำกัดแค่ช่วงแคบรอบ MgCl2-NaCl-KCl เดิม จึงครอบคลุมตัวอย่างที่ราวๆ ต่ำกว่า/สูงกว่าช่วงเดิมได้ด้วย
//   (ข) ใช้ "ตัวทำนายจุดสมดุล" แบบเดียวกับโหมด Predict บนเครื่อง (fitAR1() ที่มีอยู่แล้วในไฟล์นี้ — ฟิต y[n+1]=a+k*y[n]
//       จากตัวอย่าง raw ที่เก็บไว้) มาช่วยเดาว่าค่าดิบ "กำลังจะไปจบที่ไหน" ก่อนที่กราฟจะเข้าสมดุลจริง แทนที่จะดูแค่
//       ความชันปัจจุบัน (ซึ่งจะ "รู้ทีหลัง" พอกราฟเริ่มโค้งชะลอเข้าสมดุล) ช่วยให้ของที่พุ่งขึ้นเร็วแบบเอ็กซ์โพเนนเชียล
//       (เช่น วุ้นเส้น/น้ำ) ถูกจัดเข้าโซนสูงได้ไวขึ้น และของที่แนวโน้มจะไปจบต่ำ (ของแห้ง) ไม่ถูกดึงขึ้นเกินจริง
//  "โซน" เป็นเลขต่อเนื่อง z = 0..3 (ไม่ใช่ขั้นบันได): z คำนวณจากความชัน+จุดสมดุลที่ทำนาย แล้วขยับเข้าหาเป้าแบบจำกัดอัตรา
//  (ขึ้นเร็ว 0.02/วิ ลงช้า 0.005/วิ = ฮิสเทอรีซิส) จึงเนียนและไม่ตกกลับทันทีเมื่อกราฟชะลอชั่วคราว
//  สูตร:  aw_out = aw_table + w * (target(z) - aw_table)    โดย w = ตัวถ่วง (ความมั่นใจจากจำนวนตัวอย่าง x ขอบหน้าต่าง raw)
//  ค่าที่ใช้คำนวณทั้งหมด (base / target / w / offset / z / slope / จุดสมดุลที่ทำนาย) ถูกส่งขึ้นเว็บใน /data ให้กราฟ Offset
//  ของแอดมินวาดสูตร+error ฟังก์ชันเดิม updateSubstanceGuess / applySubstanceGuessBias / resetSubstanceGuessWindow ยังใช้ชื่อเดิม
// ============================================================================
// v-cal-2026-09-29b: สวิตช์หลักของระบบ "เดาสารจากแนวโน้ม" (offset ตามโซน + เริ่มวัดด้วยค่าดิบแล้วค่อยผสม)
//   0 = ปิด — aw ที่แสดง = ตารางคาลิเบรต x ชดเชยอุณหภูมิ ล้วน ๆ (ห้องเปล่า/ตัวอย่างนิ่งไม่ถูกดึงขึ้นลง และไม่มีช่วง "เด้ง" ตอนเริ่มวัด)
//   1 = เปิดแบบเดิม (โซน 0 ดึงค่าลงได้สูงสุด 0.10 เมื่อ raw นิ่ง / 15 วิแรกแสดง raw ล้วน แล้วค่อยผสมเข้าหาค่าคาลิเบรตใน 75 วิ)
#define TREND_OFFSET_ENABLE 0
const float TO_RAW_LOW  = 0.491f, TO_AW_LOW  = 0.328f;   // anchor MgCl2 (วัดจริง 09-25)
const float TO_RAW_MID  = 0.611f, TO_AW_MID  = 0.753f;   // anchor NaCl  (วัดจริง 09-25)
const float TO_RAW_HIGH = 0.635f, TO_AW_HIGH = 0.843f;   // KCl (ประมาณ ตรงกับ CAL_POINTS_FACTORY)
const float TO_RAW_PURE = 1.000f, TO_AW_PURE = 1.000f;   // น้ำบริสุทธิ์ (aw=1 ที่ raw=1 โดยนิยาม) — ปลายเส้นโซน 3
// v2: ขยายหน้าต่างที่ระบบยอมปรับให้กว้างขึ้น (เดิม 0.42-0.69) ครอบคลุมของแห้ง (มาม่า/ขนม, raw ต่ำ) ถึงของเปียกจัด (น้ำ/วุ้นเส้น, raw สูง)
const float TO_WIN_START = 0.180f, TO_WIN_FULL_LO = 0.470f;  // raw ต่ำกว่า START = ไม่ยุ่ง (เชื่อตารางล้วน), START->FULL_LO ค่อย ๆ เพิ่มน้ำหนัก
const float TO_WIN_FULL_HI = 0.660f, TO_WIN_END = 0.860f;    // FULL_HI->END ค่อย ๆ ลดน้ำหนักจนเป็น 0 (สูงกว่านี้ใกล้อิ่มตัวมาก เชื่อตารางล้วน)
const float TO_NUDGE_MAX = 0.03f;        // v-nudge: ขยับสูงสุด +/- นี้ (aw) เมื่ออยู่กลางช่วง (ไม่แห้งจัด/ไม่ชุ่มจัด)
const float TO_EXTREME_MAX = 0.10f;      // v-nudge: ขยับสูงสุดเมื่อ aw อยู่ปลายสุดจริง ๆ
const float TO_NUDGE_FULL_DAW = 0.05f;   // v-nudge: จุดสมดุลที่ทำนายต่างจากค่าตอนนี้เท่านี้ (aw) = ทิศทางเต็ม 100%
const float TO_DRY_START_AW = 0.45f, TO_DRY_FULL_AW = 0.30f;   // aw ต่ำกว่า START เริ่มนับว่าแห้ง, ถึง FULL = แห้งจัดเต็มที่
const float TO_WET_START_AW = 0.80f, TO_WET_FULL_AW = 0.95f;   // aw สูงกว่า START เริ่มนับว่าชุ่ม, ถึง FULL = ชุ่มจัดเต็มที่
const float TO_LOW_CAP = 0.10f;          // โซน 0: ปรับได้ไม่เกิน +/- นี้
const float TO_MID_MAX_AW = 0.75f;       // โซน 1: เพดาน aw
const float TO_HIGH_MAX_AW = 0.90f;      // โซน 2: เพดาน aw
const float TO_VHIGH_MAX_AW = 0.99f;     // v2 โซน 3: เพดาน aw (ของเปียกจัด/น้ำ — กันไม่ให้ชนกันสมบูรณ์ 1.000 พอดี)
const float TO_OFF_ABS_MAX = 0.25f;      // กันชั้นสุดท้าย: offset รวมไม่เกิน +/- นี้เสมอ
const float TO_SLOPE_FLAT = 0.004f;      // raw/นาที ต่ำกว่านี้ = นิ่ง/ลง -> โซน 0
const float TO_SLOPE_MID  = 0.014f;      // ~ 0.05 aw/นาที เดิม -> z = 1
const float TO_SLOPE_HIGH = 0.035f;      // ขึ้นเร็วต่อเนื่อง -> z = 2
const float TO_SLOPE_VHIGH = 0.070f;     // v2: ขึ้นเร็วมาก/ต่อเนื่องไม่หยุด -> z = 3 (เช่น วุ้นเส้น/น้ำ)
const float TO_Z_UP_PER_S = 0.020f;      // z ขึ้นได้เร็วสุดต่อวินาที
const float TO_Z_DOWN_PER_S = 0.005f;    // z ลงได้ช้ากว่า (ฮิสเทอรีซิส)
const unsigned long TO_SAMPLE_MS = 2000UL;  // สุ่มเก็บทุก 2 วิ (ไม่ขึ้นกับว่ามีกี่เครื่องโพล /data)
#define TO_BUF 45                        // 45 ตัวอย่าง x 2 วิ = หน้าต่าง 90 วิ
const int TO_MIN_SAMPLES = 15;           // ต่ำกว่านี้ w = 0 (ยังไม่แน่ใจ ไม่ปรับ)
const int TO_FULL_SAMPLES = 30;          // ตั้งแต่นี้ความมั่นใจเต็ม
const int TO_EQ_MIN_SAMPLES = 10;        // v2: ต้องมีตัวอย่างอย่างน้อยเท่านี้ก่อนลองฟิต fitAR1() หาจุดสมดุล
const float TO_EQ_MAX_JUMP = 0.35f;      // v2: จุดสมดุลที่ฟิตได้ ห่างจากค่าล่าสุดเกินนี้ถือว่าฟิตพัง ไม่ใช้
const float TO_EQ_WEIGHT = 0.55f;        // v2: น้ำหนักของ "โซนจากจุดสมดุลที่ทำนาย" เทียบกับ "โซนจากความชันสด" เมื่อฟิตสำเร็จ

float toSlope = 0.0f;      // ความชัน raw (raw/นาที) หลังเฉลี่ย EMA
float toZ = 0.0f;          // โซนต่อเนื่อง 0..3
float toW = 0.0f;          // ตัวถ่วงรวม (0..1) ของการเรียกล่าสุด
float toBase = NAN;        // aw จากตารางก่อนปรับ (การเรียกล่าสุด)
float toTarget = NAN;      // aw เป้าหมายของโซนปัจจุบัน
float toOff = 0.0f;        // offset ที่ใช้จริง = aw_out - aw_table
float toEqRaw = NAN;       // v2: ค่าดิบที่ทำนายว่าจะไปจบ (จาก fitAR1 บนหน้าต่าง 90 วิ) — NAN = ยังฟิตไม่ได้/ไม่ใช้
bool toEqUsed = false;     // v2: จุดสมดุลที่ทำนายถูกใช้ช่วยตัดสินโซนในการเรียกล่าสุดหรือไม่ (ให้เว็บโชว์ตัวบอกความมั่นใจ)
static float toBufRaw[TO_BUF], toBufT[TO_BUF];
static int toN = 0;
static unsigned long toT0Ms = 0, toLastSampleMs = 0, toLastMeasureStartMs = 0xFFFFFFFFUL;
static float toSlopeEma = 0.0f;
static bool toEmaInit = false;

static float toSmooth01(float x) { if (x <= 0.0f) return 0.0f; if (x >= 1.0f) return 1.0f; return x * x * (3.0f - 2.0f * x); }
static float toLineLow(float raw)   { return raw * (TO_AW_LOW / TO_RAW_LOW); }
static float toLineMid(float raw)   { return TO_AW_LOW + (TO_AW_MID - TO_AW_LOW) / (TO_RAW_MID - TO_RAW_LOW) * (raw - TO_RAW_LOW); }
static float toLineHigh(float raw)  { return TO_AW_MID + (TO_AW_HIGH - TO_AW_MID) / (TO_RAW_HIGH - TO_RAW_MID) * (raw - TO_RAW_MID); }
static float toLineVHigh(float raw) { return TO_AW_HIGH + (TO_AW_PURE - TO_AW_HIGH) / (TO_RAW_PURE - TO_RAW_HIGH) * (raw - TO_RAW_HIGH); }  // v2: KCl -> น้ำบริสุทธิ์

// v2: แปลงค่าดิบ (จุดสมดุลที่ทำนาย หรือค่าสดก็ได้) เป็น "โซนต่อเนื่อง" 0..3 โดยใช้ตำแหน่ง raw เทียบกับ 3 anchor เดียวกัน
// ใช้ทั้งตอนแปลงจุดสมดุลที่ทำนาย (toEqRaw) และช่วยให้โค้ดสองจุดไม่ต้องคัดลอกตรรกะซ้ำกัน
static float toRawToZone(float raw) {
  if (raw <= TO_RAW_LOW) return 0.0f;
  if (raw <= TO_RAW_MID) return (raw - TO_RAW_LOW) / (TO_RAW_MID - TO_RAW_LOW);
  if (raw <= TO_RAW_HIGH) return 1.0f + (raw - TO_RAW_MID) / (TO_RAW_HIGH - TO_RAW_MID);
  return 2.0f + constrain((raw - TO_RAW_HIGH) / (TO_RAW_PURE - TO_RAW_HIGH), 0.0f, 1.0f);
}

// aw เป้าหมายของโซนจำนวนเต็ม (0/1/2/3) ที่ raw นี้ เทียบกับค่าตาราง base
static float toZoneTarget(int zone, float raw, float base) {
  if (zone <= 0) return base + constrain(toLineLow(raw) - base, -TO_LOW_CAP, TO_LOW_CAP);
  if (zone == 1) return fminf(toLineMid(raw), TO_MID_MAX_AW);
  if (zone == 2) return fminf(toLineHigh(raw), TO_HIGH_MAX_AW);
  return fminf(toLineVHigh(raw), TO_VHIGH_MAX_AW);
}

// ============================================================================
//  v29: ค่า aw ที่ "แสดง/บันทึก" มีทางคำนวณทางเดียว (awShownFromRaw) ใช้ร่วมกันทั้งจอเครื่อง / เว็บ /data / ค่าที่ล็อก
//  ปัญหาเดิม (เห็นจากภาพหน้าจอ): (1) จอเครื่องใช้ค่าจากตารางล้วน แต่เว็บบวก offset โซน -> ตัวเลข/กราฟไม่ตรงกัน และค่าที่ล็อก
//  ขึ้นกับว่ามีเว็บเปิดโพลอยู่หรือไม่ (2) offset ตามโซนกระโดดตามความชัน (สูงสุด -0.25) -> aw หลังปรับตกทั้งที่ raw กำลังขึ้น
//  แก้: (ก) เริ่มวัดด้วย "ค่าดิบ" (RH/100) นิ่งไว้ TO_START_HOLD_S วิ แล้วค่อย ๆ ผสมไปหา aw ที่คาลิเบรต+ปรับแล้วตลอด TO_START_RAMP_S วิ
//       (ข) offset ขยับได้ไม่เกิน TO_OFF_SLEW_PER_S ต่อวินาที (ค) กันทิศ: raw กำลังขึ้น -> offset ห้ามลด / raw กำลังลง -> ห้ามเพิ่ม
//       (เมื่อความชัน EMA เกิน TO_DIR_DEADBAND) เพราะ aw = ตาราง(raw) + offset และตารางเป็นฟังก์ชันเพิ่มตาม raw จึงรับประกันว่า aw ไม่วิ่งสวนทาง raw
//       ทุกอย่างปรับ/ปิดได้ที่ค่าคงที่ด้านล่าง (TO_START_HOLD_S = TO_START_RAMP_S = 0 -> ไม่เริ่มจากค่าดิบ ; TO_OFF_SLEW_PER_S = 9 -> ไม่จำกัดอัตรา)
// ============================================================================
const float TO_START_HOLD_S = 0.0f;        // v-nudge: 0 = ไม่เริ่มจากค่าดิบ (ไม่มีช่วงเด้งตอนเริ่มวัด)
const float TO_START_RAMP_S = 0.0f;        // v-nudge: 0 = แสดงค่าที่คาลิเบรตตั้งแต่วินาทีแรก
const float TO_OFF_SLEW_PER_S = 0.0015f;   // offset เปลี่ยนได้ไม่เกินนี้ (aw) ต่อวินาที (~0.09 ต่อนาที)
const float TO_DIR_DEADBAND = 0.004f;      // raw/นาที — ความชัน EMA เกินนี้ (ขึ้น/ลง) ถือว่ามีทิศชัด -> ใช้ตัวกันทิศ
const float TO_DIR_TOL = 0.0002f;          // ยอมให้ offset สวนทิศได้นิดเดียวต่อครั้ง (กัน noise ตรึงค่า)
static float toOffPrev = 0.0f;
static bool  toOffInit = false;
static unsigned long toApplyPrevMs = 0;
float toShown = NAN;                       // aw ที่แสดงล่าสุด (ค่าเดียวกับจอเครื่อง)
unsigned long toShownMs = 0;
float toStartMix = 0.0f;                   // 0 = ค่าดิบ, 1 = ค่าที่ปรับแล้วเต็มที่

void resetSubstanceGuessWindow() {
  toN = 0; toT0Ms = 0; toLastSampleMs = 0;
  toSlopeEma = 0.0f; toEmaInit = false;
  toSlope = 0.0f; toZ = 0.0f; toW = 0.0f; toOff = 0.0f; toBase = NAN; toTarget = NAN;
  toEqRaw = NAN; toEqUsed = false;
  toOffPrev = 0.0f; toOffInit = false; toApplyPrevMs = 0; toStartMix = 0.0f; toShown = NAN;
}

// เรียกทุกครั้งที่ /data ถูกดึง (และจากลูปวัด) — เก็บตัวอย่างทุก 2 วิ, คำนวณความชัน+จุดสมดุลที่ทำนาย แล้วขยับ z
void updateSubstanceGuess(float rawNow, unsigned long measureStartMsNow) {
#if !TREND_OFFSET_ENABLE
  (void)rawNow; (void)measureStartMsNow; return;
#endif
  if (measureStartMsNow != toLastMeasureStartMs) {   // เริ่มวัดรอบใหม่/สลับตัวอย่าง -> เริ่มนับใหม่หมด
    toLastMeasureStartMs = measureStartMsNow;
    resetSubstanceGuessWindow();
  }
  if (isnan(rawNow)) return;
  unsigned long now = millis();
  if (toN > 0 && (now - toLastSampleMs) < TO_SAMPLE_MS) return;
  float dtS = (toN > 0) ? (now - toLastSampleMs) / 1000.0f : 0.0f;
  if (dtS > 10.0f) dtS = 10.0f;
  toLastSampleMs = now;
  if (toN == 0) toT0Ms = now;
  if (toN >= TO_BUF) {
    for (int i = 1; i < TO_BUF; i++) { toBufRaw[i - 1] = toBufRaw[i]; toBufT[i - 1] = toBufT[i]; }
    toN = TO_BUF - 1;
  }
  toBufRaw[toN] = rawNow;
  toBufT[toN] = (now - toT0Ms) / 60000.0f;   // นาที
  toN++;
  if (toN < 6) return;                        // ตัวอย่างน้อยเกินไปสำหรับความชัน

  // ความชันกำลังสองน้อยสุด (raw ต่อนาที)
  float mt = 0, mr = 0;
  for (int i = 0; i < toN; i++) { mt += toBufT[i]; mr += toBufRaw[i]; }
  mt /= toN; mr /= toN;
  float sxy = 0, sxx = 0;
  for (int i = 0; i < toN; i++) { float dx = toBufT[i] - mt; sxy += dx * (toBufRaw[i] - mr); sxx += dx * dx; }
  float s = (sxx > 1e-9f) ? (sxy / sxx) : 0.0f;
  if (!toEmaInit) { toSlopeEma = s; toEmaInit = true; } else { toSlopeEma = 0.7f * toSlopeEma + 0.3f * s; }
  toSlope = toSlopeEma;

  // ความชัน -> z เป้าหมายจากแนวโน้มสด (ต่อเนื่อง 0..3)
  float ztSlope;
  if (toSlope <= TO_SLOPE_FLAT) ztSlope = 0.0f;
  else if (toSlope < TO_SLOPE_MID) ztSlope = (toSlope - TO_SLOPE_FLAT) / (TO_SLOPE_MID - TO_SLOPE_FLAT);
  else if (toSlope < TO_SLOPE_HIGH) ztSlope = 1.0f + (toSlope - TO_SLOPE_MID) / (TO_SLOPE_HIGH - TO_SLOPE_MID);
  else if (toSlope < TO_SLOPE_VHIGH) ztSlope = 2.0f + (toSlope - TO_SLOPE_HIGH) / (TO_SLOPE_VHIGH - TO_SLOPE_HIGH);
  else ztSlope = 3.0f;

  // v2: ลองทำนาย "จุดสมดุล" ที่ raw กำลังจะไปจบ ด้วยตัวฟิตเดียวกับโหมด Predict บนเครื่อง (fitAR1, นิยามท้ายไฟล์)
  // ถ้าฟิตสำเร็จและค่าที่ได้สมเหตุสมผล (อยู่ใน 0-1 และไม่กระโดดไปไกลเกินจริง) จะถ่วงเข้ากับ ztSlope ให้ตัดสินโซนไวขึ้น
  // สำหรับกราฟที่พุ่งแบบเอ็กซ์โพเนนเชียล (เช่น วุ้นเส้น/น้ำ) และไม่ดึงของแห้งขึ้นเกินจริงตอนความชันสดยังดูสูงอยู่ชั่วคราว
  float zt = ztSlope;
  toEqUsed = false;
  if (toN >= TO_EQ_MIN_SAMPLES) {
    float eqRaw, eqTau;
    if (fitAR1(toBufRaw, toN, eqRaw, eqTau) && eqRaw >= 0.0f && eqRaw <= 1.0f && fabs(eqRaw - toBufRaw[toN - 1]) <= TO_EQ_MAX_JUMP) {
      toEqRaw = eqRaw;
      float ztEq = toRawToZone(eqRaw);
      zt = (1.0f - TO_EQ_WEIGHT) * ztSlope + TO_EQ_WEIGHT * ztEq;
      toEqUsed = true;
    }
  }
  // จำกัดอัตราการขยับ: ขึ้นเร็ว ลงช้า
  if (zt > toZ) { toZ += fminf(zt - toZ, TO_Z_UP_PER_S * dtS); }
  else          { toZ -= fminf(toZ - zt, TO_Z_DOWN_PER_S * dtS); }
  toZ = constrain(toZ, 0.0f, 3.0f);
}

// เรียกหลัง applyCal() — คืน aw หลังปรับ offset ตามโซน (นอกหน้าต่าง raw หรือข้อมูลยังไม่พอ = คืนค่าตารางตรง ๆ)
static float toWindowWeight(float raw) {
  return toSmooth01((raw - TO_WIN_START) / (TO_WIN_FULL_LO - TO_WIN_START))
       * (1.0f - toSmooth01((raw - TO_WIN_FULL_HI) / (TO_WIN_END - TO_WIN_FULL_HI)));
}
// offset ที่ "อยากได้" ตามโซน zUse + น้ำหนัก wUse (ไม่มี state)
// v-bands-2026-09-29: แบ่งช่วง aw ละเอียด 10 โหนด (ตรงกับเกลืออ้างอิง LiCl/CH3COOK/MgCl2/K2CO3/NaBr/NaCl/KCl/K2SO4 + น้ำ)
//   แต่ละโหนดกำหนด 2 ค่า แล้วเทียบสัดส่วนเชิงเส้นระหว่างโหนด (ไม่เป็นขั้นบันได):
//   CAP = ขยับ aw ได้สูงสุดกี่หน่วยที่ช่วงนั้น (ขนม/มาม่า aw 0.2-0.4 และน้ำ aw >0.9 ให้ขยับได้มากกว่ากลางช่วง)
//   DAW = จุดสมดุลที่ทำนายต่างจากค่าตอนนี้เท่านี้ (aw) ถือเป็น "ทิศทางเต็ม" (ช่วงแห้ง raw ขยับช้า -> ใช้ค่าน้อยกว่า)
//   แก้ตัวเลข 3 แถวนี้เพื่อจูนโซนแต่ละช่วงได้อิสระ (ต้อง aw เรียงจากน้อยไปมาก)
#define TO_NBANDS 10
static const float TO_BAND_AW [TO_NBANDS] = { 0.00f, 0.11f, 0.23f, 0.33f, 0.43f, 0.58f, 0.75f, 0.85f, 0.95f, 1.00f };
static const float TO_BAND_CAP[TO_NBANDS] = { 0.10f, 0.09f, 0.07f, 0.05f, 0.035f,0.025f,0.03f, 0.045f,0.08f, 0.10f };
static const float TO_BAND_DAW[TO_NBANDS] = { 0.020f,0.025f,0.030f,0.035f,0.045f,0.050f,0.050f,0.045f,0.035f,0.030f };
static float toBandInterp(const float* ys, float aw) {
  if (aw <= TO_BAND_AW[0]) return ys[0];
  for (int i = 0; i < TO_NBANDS - 1; i++) {
    if (aw <= TO_BAND_AW[i + 1]) {
      float t = (aw - TO_BAND_AW[i]) / (TO_BAND_AW[i + 1] - TO_BAND_AW[i]);
      return ys[i] + t * (ys[i + 1] - ys[i]);
    }
  }
  return ys[TO_NBANDS - 1];
}
static float toWantedOffset(float awCal, float raw, float zUse, float wUse, float* targetOut) {
#if !TREND_OFFSET_ENABLE
  (void)raw; (void)zUse; (void)wUse; if (targetOut) *targetOut = awCal; return 0.0f;
#endif
  // v-room-aw: ก่อนหน้านี้โค้ดเลือกเส้น KCl/น้ำจากความชันอย่างเดียว ทำให้ค่าในห้องหรือ
  // ค่าที่ยังเปลี่ยนอยู่ถูกดึงขึ้นไปใกล้ 0.843 ได้ ทั้งที่ยังไม่มีหลักฐานว่าเป็นตัวอย่างนั้น
  // รอบนี้ใช้ AW ห้องเป็น candidate แรก และใช้ค่าเป้าหมายของตัวอย่างที่ผู้ใช้ระบุเป็น
  // candidate อื่น จากนั้นเลือกค่าที่ใกล้กับ equilibrium ที่สังเกตได้ที่สุด
  (void)raw; (void)zUse;
  float roomAw = roomReferenceAw(currentTempC);
  if (!isnan(roomAw)) {
    float evidenceAw = awCal;
    if (toEqUsed && !isnan(toEqRaw)) {
      float eqAw = applyCal(toEqRaw, currentTempC);
      // ตารางคาลิเบรตช่วงกลางอาจขยาย raw ต่างกันเพียงเล็กน้อยเป็น aw ต่างกันมาก
      // ถ้าจุดสมดุลที่ทำนายไกลจากค่าปัจจุบันเกิน 0.10 ให้ใช้ค่าปัจจุบันแทน
      // เพื่อไม่ให้การเดาเส้นแนวโน้มครั้งเดียวเลือก "น้ำ" แล้วดันกราฟขึ้น
      if (!isnan(eqAw) && fabsf(eqAw - awCal) <= 0.10f) evidenceAw = eqAw;
    }

    float candidates[5];
    int n = 0;
    candidates[n++] = roomAw;
    candidates[n++] = AW_TARGET_CALCIUM_CHLORIDE;
    candidates[n++] = AW_TARGET_INSTANT_NOODLE;
    candidates[n++] = AW_TARGET_TABLE_SALT;
    candidates[n++] = AW_TARGET_WATER;

    int best = 0;
    float bestDist = fabsf(evidenceAw - candidates[0]);
    for (int i = 1; i < n; i++) {
      float d = fabsf(evidenceAw - candidates[i]);
      if (d < bestDist) { best = i; bestDist = d; }
    }
    // ใกล้ห้องมากพอให้ถือว่าเป็น baseline ห้องเสมอ ไม่เดาเป็นสารตัวอย่าง
    if (fabsf(evidenceAw - roomAw) <= ROOM_AW_MATCH_TOL) best = 0;

    float target = constrain(candidates[best], 0.0f, 1.0f);
    if (targetOut) *targetOut = target;

    // เมื่อมีข้อมูลมากพอ wUse จะค่อย ๆ เพิ่มจาก 0 -> 1; จึงไม่กระโดดตอนเริ่มวัด
    // ยอมแก้ได้มากกว่า TO_BAND_CAP เล็กน้อยเพื่อดึงค่าผิดแบบ 0.83 ลงมาหาเกลือแกง
    // หรือดึงค่าน้ำขึ้นหา 1.0 แต่ยังจำกัดเพดานเพื่อไม่ให้การเดาครั้งเดียวแก้ค่ารุนแรงเกินไป
    const float ROOM_TARGET_MAX_CORRECTION = 0.16f;
    return constrain(wUse * (target - awCal), -ROOM_TARGET_MAX_CORRECTION, ROOM_TARGET_MAX_CORRECTION);
  }

  // ถ้ายังไม่มี baseline ห้อง ให้ใช้พฤติกรรมเดิมเป็น fallback (ไม่เดา target จากค่าเดียว)
  float cap = toBandInterp(TO_BAND_CAP, awCal);
  float daw = toBandInterp(TO_BAND_DAW, awCal);
  float sSlope = 0.0f;
  float as = fabsf(toSlope);
  if (as > TO_SLOPE_FLAT) sSlope = constrain((as - TO_SLOPE_FLAT) / (TO_SLOPE_MID - TO_SLOPE_FLAT), 0.0f, 1.0f) * (toSlope > 0.0f ? 1.0f : -1.0f);
  float s = sSlope;
  if (toEqUsed && !isnan(toEqRaw)) {
    float sEq = constrain((calTableLookup(toEqRaw) - awCal) / daw, -1.0f, 1.0f);
    s = (1.0f - TO_EQ_WEIGHT) * sSlope + TO_EQ_WEIGHT * sEq;
  }
  float off = constrain(wUse * s * cap, -TO_OFF_ABS_MAX, TO_OFF_ABS_MAX);
  if (targetOut) *targetOut = awCal + s * cap;
  return off;
}
static float toStartMixNow() {
#if !TREND_OFFSET_ENABLE
  return 1.0f;
#endif
  if (toN == 0 || toT0Ms == 0) return 0.0f;                      // เพิ่งเริ่ม/เพิ่งเปิดฝา -> ค่าดิบ
  if (TO_START_HOLD_S <= 0.0f && TO_START_RAMP_S <= 0.0f) return 1.0f;
  float el = (millis() - toT0Ms) / 1000.0f;
  if (el <= TO_START_HOLD_S) return 0.0f;
  if (TO_START_RAMP_S <= 0.0f) return 1.0f;
  return toSmooth01((el - TO_START_HOLD_S) / TO_START_RAMP_S);
}
static float toCompose(float raw, float awCal, float off, float mix) {
  float adj = constrain(awCal + off, 0.0f, 1.0f);
  return constrain(raw * (1.0f - mix) + adj * mix, 0.0f, 1.0f);
}
// เรียกหลัง applyCal() ทุกครั้งที่มี raw ใหม่ในโหมดวัด — คืน aw ที่ "แสดง" และเดินสถานะ (slew / กันทิศ / ผสมจากค่าดิบ)
float applySubstanceGuessBias(float awFromCal, float raw) {
  toBase = awFromCal;
  unsigned long now = millis();
  if (isnan(raw) || isnan(awFromCal)) { toW = 0.0f; toOff = 0.0f; toTarget = awFromCal; toShown = awFromCal; toShownMs = now; return awFromCal; }
  float mix = toStartMixNow(); toStartMix = mix;
  float conf = constrain((toN - TO_MIN_SAMPLES) / (float)(TO_FULL_SAMPLES - TO_MIN_SAMPLES), 0.0f, 1.0f);
  toW = toWindowWeight(raw) * conf;
  float target = awFromCal;
  float want = toWantedOffset(awFromCal, raw, toZ, toW, &target);
  toTarget = target;
  float off = want;
  if (toOffInit) {
    float dt = (now - toApplyPrevMs) / 1000.0f; if (dt < 0.0f) dt = 0.0f; if (dt > 5.0f) dt = 5.0f;
    float maxStep = TO_OFF_SLEW_PER_S * dt;
    off = toOffPrev + constrain(want - toOffPrev, -maxStep, maxStep);
    if (toEmaInit && toN >= 6) {
      if (toSlope > TO_DIR_DEADBAND && off < toOffPrev - TO_DIR_TOL) off = toOffPrev - TO_DIR_TOL;         // raw ขึ้น -> aw ห้ามลงเพราะ offset
      else if (toSlope < -TO_DIR_DEADBAND && off > toOffPrev + TO_DIR_TOL) off = toOffPrev + TO_DIR_TOL;   // raw ลง -> aw ห้ามขึ้นเพราะ offset
    }
  }
  toOffPrev = off; toOffInit = true; toApplyPrevMs = now;
  float shown = toCompose(raw, awFromCal, off, mix);
  toShown = shown; toShownMs = now;
  toOff = shown - awFromCal;      // offset รวมที่มีผลจริงเทียบตาราง (เว็บใช้วาดกราฟ offset)
  return shown;
}
// ค่าที่ล็อกตอนไฟเขียว: ฐานจาก raw ที่ล็อก (จุดกึ่งกลางการแกว่ง) + offset ที่ใช้อยู่จริงตอนนี้ -> สอดคล้องกับที่แสดงอยู่
float awShownAtLock(float awCalLocked, float rawLocked) {
  return toCompose(rawLocked, awCalLocked, toOffInit ? toOffPrev : 0.0f, toStartMixNow());
}
// ค่าทำนายสมดุล (โหมด Predict) ให้อยู่สเกลเดียวกับเส้นสด — ไม่เดินสถานะ
float awShownPeek(float rawX, float tempC) {
  float awCal = applyCal(rawX, tempC);
#if AW_RAW_MODE
  return awCal;
#else
  float conf = constrain((toN - TO_MIN_SAMPLES) / (float)(TO_FULL_SAMPLES - TO_MIN_SAMPLES), 0.0f, 1.0f);
  float off = toWantedOffset(awCal, rawX, toZ, toWindowWeight(rawX) * conf, nullptr);
  return toCompose(rawX, awCal, off, toStartMixNow());
#endif
}


// ============================================================================
//  v-lid-event (2026-09-25): ตรวจจับ "ช่วงเปิดฝาเพื่อใส่/เปลี่ยนตัวอย่าง" จากการที่ raw กระโดดขึ้นหรือลง
//  เร็วผิดปกติ (อากาศห้องเข้าไปในกล่องวัดชั่วขณะตอนเปิดฝา ไม่ว่าจะเปิดแล้วชื้นขึ้นหรือแห้งลงก็ตาม) — ใช้เป็น
//  "จุดตัด" ที่แม่นกว่าการอิง measureStartMs (เวลากดปุ่ม) เพียงอย่างเดียว เพราะบางทีกดปุ่มเริ่มวัดกับเปิดฝาจริง
//  ไม่พร้อมกันเป๊ะ ๆ และกันไม่ให้ข้อมูลช่วงที่ยังกระเพื่อมจากการเปิดฝาเอง (ไม่ใช่จากตัวอย่าง) ไปปนกับการคำนวณ
//  ความชันของ substance-guess — ไม่ใช้ "ทิศทาง" ของการกระโดดมาตัดสินชนิดสาร เพราะอากาศห้องมี %RH คงที่
//  ไม่เกี่ยวกับตัวอย่างที่ใส่เข้าไป ใช้แค่บอกว่า "ช่วงนี้ยังไม่นิ่ง อย่าเพิ่งเอาไปคิด" เท่านั้น
// ============================================================================
const float LID_EVENT_RAW_JUMP     = 0.03f;    // raw เปลี่ยนเกินนี้ภายในหน้าต่างสั้น ๆ ถือว่า "กำลังเปิดฝา"
const unsigned long LID_EVENT_WINDOW_MS = 4000UL;   // หน้าต่างสั้น ๆ ที่ใช้ตรวจจับการกระโดด (4 วิ)
const unsigned long LID_EVENT_SETTLE_MS = 30000UL;  // ต้องนิ่ง (ไม่กระโดดอีก) ต่อเนื่องกี่ วิ ก่อนถือว่าผ่านช่วงเปิดฝาไปแล้ว

static float lidBufRaw[8];
static unsigned long lidBufMs[8];
static int lidBufN = 0, lidBufHead = 0;
bool lidEventActive = false;              // true = กำลังอยู่ในช่วงกระเพื่อมจากการเปิดฝา (ยังไม่น่าเชื่อถือ)
unsigned long lidEventDetectedMs = 0;

// เรียกทุกครั้งที่มีค่า raw ใหม่เข้ามา (จุดเดียวกับ updateSubstanceGuess) — คืน true ถ้ายังอยู่ในช่วงเปิดฝา/
// รอให้นิ่ง (ตัวเรียกควรข้าม ไม่เอาค่ารอบนี้ไปนับสถิติ/ตัดสินใจใด ๆ)
bool updateLidEventDetector(float rawNow, unsigned long nowMs) {
  lidBufRaw[lidBufHead] = rawNow; lidBufMs[lidBufHead] = nowMs;
  lidBufHead = (lidBufHead + 1) % 8; if (lidBufN < 8) lidBufN++;

  float mn = rawNow, mx = rawNow; // หา min/max ของ raw ภายในหน้าต่าง LID_EVENT_WINDOW_MS ล่าสุด
  for (int i = 0; i < lidBufN; i++) {
    if (nowMs - lidBufMs[i] <= LID_EVENT_WINDOW_MS) {
      if (lidBufRaw[i] < mn) mn = lidBufRaw[i];
      if (lidBufRaw[i] > mx) mx = lidBufRaw[i];
    }
  }

  if ((mx - mn) >= LID_EVENT_RAW_JUMP) {
    lidEventActive = true;
    lidEventDetectedMs = nowMs;      // ยังกระโดดอยู่ -> เลื่อนนาฬิกา "รอให้นิ่ง" ออกไปเรื่อย ๆ
  } else if (lidEventActive && (nowMs - lidEventDetectedMs) >= LID_EVENT_SETTLE_MS) {
    lidEventActive = false;          // นิ่งต่อเนื่องครบ LID_EVENT_SETTLE_MS แล้ว -> ผ่านช่วงเปิดฝาไปแล้วจริง
  }
  return lidEventActive;
}

// v-accuracy: ต้องเรียง raw จากน้อยไปมากเสมอ (ห้ามเท่ากันหรือย้อนกลับ) ไม่งั้น applyCal() หาช่วงผิดพลาด/หารด้วยศูนย์
bool isCalPointsMonotonic(const CalPoint* pts, int count) {
  for (int i = 1; i < count; i++) {
    if (pts[i].raw <= pts[i - 1].raw) return false;
  }
  return true;
}

// โหลดจุดคาลิเบรตจาก NVS (namespace แยกจาก "awrec" ที่ใช้เก็บ recording กันชนกัน) เรียกครั้งเดียวใน setup()
// ถ้ายังไม่เคยบันทึกไว้เลย (คีย์ "raw0" ไม่มี) หรือข้อมูลที่โหลดมาผิดปกติ (ไม่เรียงลำดับ) จะ fallback ไปค่าโรงงาน
// กันไม่ให้ applyCal() คำนวณผิดพลาด/หารด้วยศูนย์จากข้อมูลเสีย (เช่น NVS เพิ่งถูกล้าง หรือเขียนไม่ครบ)
void loadCalPoints() {
#if AW_RAW_MODE
  // RAW: ไม่ใช้คาลิเบรต จึงไม่โหลดจุดที่เคยบันทึกไว้จาก NVS (ไม่แตะข้อมูลเดิม ถ้าสลับกลับไปไฟล์คาลิเบรตยังอยู่ครบ)
  for (int i = 0; i < CAL_POINTS_COUNT; i++) calPoints[i] = CAL_POINTS_FACTORY[i];
  calSavedAtEpoch = 0;
  return;
#endif
  prefs.begin("awcal", true);
  bool hasSaved = prefs.isKey("raw0") && prefs.getUInt("schema", 0) == 27;
  if (hasSaved) {
    for (int i = 0; i < CAL_POINTS_COUNT; i++) {
      String rk = "raw" + String(i), ak = "aw" + String(i);
      calPoints[i].raw = prefs.getFloat(rk.c_str(), CAL_POINTS_FACTORY[i].raw);
      calPoints[i].aw = prefs.getFloat(ak.c_str(), CAL_POINTS_FACTORY[i].aw);
    }
    calSavedAtEpoch = (time_t)prefs.getULong64("savedEp", 0); // v-pro: audit trail - เวลาที่คาลิเบรตครั้งล่าสุด
  }
  prefs.end();
  if (!hasSaved || !isCalPointsMonotonic(calPoints, CAL_POINTS_COUNT)) {
    for (int i = 0; i < CAL_POINTS_COUNT; i++) calPoints[i] = CAL_POINTS_FACTORY[i];
    if (hasSaved) Serial.println("[CAL] Saved calibration was invalid (not monotonic) - reverted to factory defaults");
  }
}

// ตรวจสอบ+บันทึกจุดคาลิเบรตชุดใหม่ลง NVS แล้วสลับมาใช้งานทันที (ไม่ต้องรีบูต)
// คืนค่า false พร้อมข้อความ error ใน errOut ถ้าข้อมูลไม่ผ่านการตรวจสอบ (ไม่เรียงลำดับ หรือค่านอกช่วง 0-1)
bool saveCalPoints(const CalPoint* pts, int count, String& errOut) {
  if (count != CAL_POINTS_COUNT) { errOut = "point count mismatch"; return false; }
  for (int i = 0; i < count; i++) {
    if (isnan(pts[i].raw) || isnan(pts[i].aw) || pts[i].raw < 0.0 || pts[i].raw > 1.0 || pts[i].aw < 0.0 || pts[i].aw > 1.0) {
      errOut = "point " + String(i) + " out of range 0..1";
      return false;
    }
  }
  if (!isCalPointsMonotonic(pts, count)) {
    errOut = "raw values must strictly increase from point 0 to " + String(count - 1);
    return false;
  }
  prefs.begin("awcal", false);
  for (int i = 0; i < count; i++) {
    String rk = "raw" + String(i), ak = "aw" + String(i);
    prefs.putFloat(rk.c_str(), pts[i].raw);
    prefs.putFloat(ak.c_str(), pts[i].aw);
  }
  calSavedAtEpoch = nowEpoch(); // v-pro: จำเวลาที่คาลิเบรตไว้ ใช้เตือน "คาลิเบรตเกินอายุ" บน System Health
  prefs.putULong64("savedEp", (uint64_t)calSavedAtEpoch);
  prefs.putUInt("schema", 27);
  prefs.end();
  for (int i = 0; i < count; i++) calPoints[i] = pts[i];
  Serial.println("[CAL] New calibration points saved and applied");
  return true;
}

// ---------- v-pro: นาฬิกา audit trail (ดูคำอธิบายที่จุดประกาศตัวแปร clockEpochAtSync ด้านบนไฟล์) ----------
// คืนเวลาปัจจุบันแบบ epoch (วินาทีนับจาก 1970) โดยประมาณจากจังหวะซิงก์ล่าสุด + millis() ที่ผ่านไป
// คืนค่า 0 ถ้ายังไม่เคยซิงก์เวลาเลยตั้งแต่เครื่องผลิตมา (กรณีนี้ไม่ควรเกิดหลังใช้งานเว็บครั้งแรก)
time_t nowEpoch() {
  if (!clockEverSynced) return 0;
  return clockEpochAtSync + (time_t)((millis() - clockMillisAtSync) / 1000UL);
}

// จัดรูปแบบเวลาเป็น "YYYY-MM-DD HH:MM" (UTC) สำหรับแนบไปกับ audit trail/หน้าจอ ถ้ายังไม่เคยซิงก์จะคืน "unsynced"
void formatEpoch(time_t ep, char* out, size_t outSize) {
  if (ep == 0) {
    snprintf(out, outSize, "unsynced");
    return;
  }
  time_t t = ep;
  struct tm* g = gmtime(&t);
  snprintf(out, outSize, "%04d-%02d-%02d %02d:%02d", g->tm_year + 1900, g->tm_mon + 1, g->tm_mday, g->tm_hour, g->tm_min);
}

// โหลดเวลาที่เคยซิงก์ไว้ล่าสุดจาก NVS ตอนบูต (ค่าที่ได้เป็นแค่ "อย่างน้อยเท่านี้" ไม่ใช่เวลาจริง ณ ขณะนี้
// เพราะไม่รู้ว่าไฟดับไปนานเท่าไหร่ระหว่างทาง) — clockVerifiedThisBoot ยังเป็น false จนกว่าจะซิงก์ผ่านเว็บจริง
void loadClockFromNVS() {
  prefs.begin("awclock", true);
  uint64_t saved = prefs.getULong64("epoch", 0);
  String opSaved = prefs.getString("op", "");
  prefs.end();
  if (saved > 0) {
    clockEpochAtSync = (time_t)saved;
    clockMillisAtSync = millis();
    clockEverSynced = true;
  }
  opSaved.toCharArray(operatorTag, sizeof(operatorTag));
}

// เรียกทุกครั้งที่เบราว์เซอร์ส่งเวลาปัจจุบันมาให้ (ดู handleClockSync()) — บันทึกลง NVS ด้วยเพื่อให้รอบบูต
// ถัดไปยังมีค่าประมาณเวลาไว้ใช้ก่อนซิงก์ใหม่ (ดีกว่าเริ่มนับจาก 0 ทุกครั้งเหมือน millis() เดิม)
void syncClockFromWeb(time_t epochFromBrowser) {
  clockEpochAtSync = epochFromBrowser;
  clockMillisAtSync = millis();
  clockEverSynced = true;
  clockVerifiedThisBoot = true;
  prefs.begin("awclock", false);
  prefs.putULong64("epoch", (uint64_t)epochFromBrowser);
  prefs.end();
}

void saveOperatorTag(const char* tag) {
  strncpy(operatorTag, tag, sizeof(operatorTag) - 1);
  operatorTag[sizeof(operatorTag) - 1] = '\0';
  prefs.begin("awclock", false);
  prefs.putString("op", operatorTag);
  prefs.end();
}

// ---------- v14: ชดเชยความต่างอุณหภูมิ ตัวชิป SHT vs ตัวอย่าง ----------
extern float shtTempC;
extern unsigned long lastShtTempMs;
float psatKPa(float tC) { return 0.61094f * expf(17.625f * tC / (tC + 243.04f)); }   // สูตร Magnus (Alduchov-Eskridge) หน่วย kPa
extern float currentTempC;
float gradientDeltaC() {    // บวก = ตัวชิปอุ่นกว่าตัวอย่าง (ค่า RH จะอ่านต่ำกว่าจริง)  NAN = ยังไม่มีข้อมูลเซนเซอร์ครบ
  if (isnan(shtTempC) || isnan(currentTempC) || millis() - lastShtTempMs > 5000UL) return NAN;
  return shtTempC - currentTempC - getEffectiveShtDsOffsetC();
}
// ตัวคูณ Psat(T_ชิป)/Psat(T_ตัวอย่าง) — deadband แบบต่อเนื่อง (ส่วนต่างที่เกิน GRADIENT_DEADBAND_C เท่านั้นที่ถูกชดเชย) และจำกัด GRADIENT_MAX_C
// v-cal-priority: ลำดับ 2) ของ 3 ลำดับความสำคัญการคาลิเบต (ดูคำอธิบายเต็มที่ applyCal()) — คูณทับผลลัพธ์ของ
// ลำดับ 3 (calTableLookup) เสมอ คืน 1.0 อัตโนมัติเมื่อข้อมูลเซนเซอร์ไม่พอ (เท่ากับ fallback ไปใช้ลำดับ 3 ล้วน ๆ)
// v25: ดีดแบนด์แบบปรับตามข้อมูลจริง — offset ยังไม่น่าเชื่อถือ = เพดานเดิม (0.5 °C) / เชื่อถือได้แล้ว = MIN + K x sd (ไม่เกินเพดาน)
float gradientDeadbandC() {
  if (!learnedOffsetTrusted || isnan(learnedOffsetSdC)) return GRADIENT_DEADBAND_C;
  float db = GRADIENT_DEADBAND_MIN_C + OFFSET_DB_K * learnedOffsetSdC;
  if (db < GRADIENT_DEADBAND_MIN_C) db = GRADIENT_DEADBAND_MIN_C;
  if (db > GRADIENT_DEADBAND_C) db = GRADIENT_DEADBAND_C;
  return db;
}
float gradientFactor() {
  if (!TEMP_GRADIENT_CORRECTION) return 1.0f;
  float d = gradientDeltaC();
  if (isnan(d)) return 1.0f;
  float a = fabs(d);
  float db = gradientDeadbandC();
  if (a <= db) return 1.0f;
  a -= db;
  if (a > GRADIENT_MAX_C) a = GRADIENT_MAX_C;
  float ts = currentTempC;
  float td = ts + ((d > 0) ? a : -a);
  return psatKPa(td) / psatKPa(ts);
}

// ============================================================================
//  v27: พื้นผิวคาลิเบรต 2 มิติ  aw = ตาราง(raw) x gradientFactor() x surfaceFactor(aw, T)
//  - ตาราง 1 มิติ (calTableLookup) ยังเป็น "ตัวอ้างอิงที่ 25 °C" เหมือนเดิมทุกประการ ไม่ถูกแก้
//  - พื้นผิวเก็บเฉพาะ "ตัวคูณแก้ตามอุณหภูมิ" ที่สมการเดิม (ตาราง + Magnus) ยังทำนายคลาดเมื่อเทียบกับ 25 °C ของตัวอย่างเดียวกัน
//    จุดอุณหภูมิ 3 จุด (19 / 20 / 25 °C; 25 °C = 1.0 เสมอ) x จุด aw 6 จุด (AW_GRID) — ค่าตั้งต้นทุกช่อง = 1.0 => ผลเท่าเดิมเป๊ะ
//  - ได้ข้อมูลจากโหมดคาลิเบตอัตโนมัติ (ช่อง 25/20/19 °C) เมื่อแอดมินสั่งบันทึกเองเท่านั้น (/admin/calsurface/commit) ไม่เขียนเองอัตโนมัติ
//  - นอกช่วงข้อมูล: T >= 25 °C = 1.0 / T < 19 °C = ค้างที่ค่า 19 °C (ไม่ extrapolate) / aw ต่ำกว่าจุดวัดจริงค่อย ๆ ลู่เข้า 1.0 ที่ aw = 0
// ============================================================================
#define SURF_T_N 2
#define SURF_AW_N 6
const float SURF_T_NODE[SURF_T_N] = { 19.0f, 20.0f };                    // จุดอุณหภูมิที่มีข้อมูล (25 °C = 1.0 คงที่)
const float SURF_T_REF = 25.0f;
const float SURF_AW_GRID[SURF_AW_N] = { 0.00f, 0.33f, 0.55f, 0.75f, 0.85f, 1.00f };   // จุด aw (จุด 0.00 ตรึงที่ 1.0 เสมอ)
const float SURF_K_MIN = 0.90f, SURF_K_MAX = 1.10f;                       // ตัวคูณที่ยอมรับ (เกินนี้ถือว่าข้อมูลผิดปกติ ไม่บันทึก)
float surfK[SURF_T_N][SURF_AW_N];
bool  surfValid = false;          // true = มีพื้นผิวที่บันทึกไว้และใช้งานอยู่
uint32_t surfCommitCount = 0;

static void surfResetToUnity() {
  for (int i = 0; i < SURF_T_N; i++) for (int j = 0; j < SURF_AW_N; j++) surfK[i][j] = 1.0f;
  surfValid = false;
}
static float surfInterpAw(const float* row, float aw) {
  if (aw <= SURF_AW_GRID[0]) return row[0];
  if (aw >= SURF_AW_GRID[SURF_AW_N - 1]) return row[SURF_AW_N - 1];
  for (int j = 0; j < SURF_AW_N - 1; j++) {
    if (aw <= SURF_AW_GRID[j + 1]) {
      float t = (aw - SURF_AW_GRID[j]) / (SURF_AW_GRID[j + 1] - SURF_AW_GRID[j]);
      return row[j] + t * (row[j + 1] - row[j]);
    }
  }
  return 1.0f;
}
float surfaceFactor(float aw, float tempC) {
#if AW_RAW_MODE
  (void)aw; (void)tempC; return 1.0f;
#else
  if (!surfValid || isnan(aw) || isnan(tempC) || tempC >= SURF_T_REF) return 1.0f;
  float k0 = surfInterpAw(surfK[0], aw);   // 19 °C
  float k1 = surfInterpAw(surfK[1], aw);   // 20 °C
  if (tempC <= SURF_T_NODE[0]) return k0;
  if (tempC <= SURF_T_NODE[1]) return k0 + (k1 - k0) * (tempC - SURF_T_NODE[0]) / (SURF_T_NODE[1] - SURF_T_NODE[0]);
  return k1 + (1.0f - k1) * (tempC - SURF_T_NODE[1]) / (SURF_T_REF - SURF_T_NODE[1]);   // 20 -> 25 °C ลู่เข้า 1.0
#endif
}

// v-room-aw: ค่า AW อ้างอิงของห้อง ณ อุณหภูมิที่กำลังวัด
// ใช้ RH/100 ของ baseline ที่อ่านก่อนมีตัวอย่าง (หรือค่าที่โหลดจากบูตก่อนหน้า) โดยตรง
// ไม่ส่งผ่าน calPoints เพราะตารางคาลิเบรตที่เคยบันทึกค้างไว้เป็นสาเหตุหนึ่งที่ทำให้ค่าห้อง
// ถูกยกไปใกล้ 0.83 ได้ และไม่ใช้ currentAw/กราฟปัจจุบัน เพราะสองค่านั้นอาจถูกตัวอย่างดึงขึ้นไปแล้ว
float roomReferenceAw(float tempC) {
  (void)tempC;
  if (isnan(ambientRHatBoot) || ambientRHatBoot < 0.0f || ambientRHatBoot > 100.0f) return NAN;
  float roomRaw = constrain(ambientRHatBoot / 100.0f, 0.0f, 1.0f);
  return roomRaw;
}

void saveCalSurface() {
  prefs.begin("awsurf", false);
  prefs.putBytes("k", surfK, sizeof(surfK));
  prefs.putUInt("cnt", surfCommitCount);
  prefs.putBool("ok", surfValid);
  prefs.end();
}
void loadCalSurface() {
  surfResetToUnity();
#if !AW_RAW_MODE
  prefs.begin("awsurf", true);
  bool ok = prefs.getBool("ok", false);
  bool sizeOk = ok && prefs.getBytesLength("k") == sizeof(surfK);
  if (sizeOk) {
    prefs.getBytes("k", surfK, sizeof(surfK));
    surfCommitCount = prefs.getUInt("cnt", 0);
  }
  prefs.end();
  if (sizeOk) {
    bool sane = true;
    for (int i = 0; i < SURF_T_N; i++) for (int j = 0; j < SURF_AW_N; j++)
      if (isnan(surfK[i][j]) || surfK[i][j] < SURF_K_MIN || surfK[i][j] > SURF_K_MAX) sane = false;
    if (sane) surfValid = true; else { surfResetToUnity(); Serial.println("[SURF] stored surface failed sanity check - ignored (factors = 1.0)"); }
  }
  Serial.printf("[SURF] surface %s (commits=%u)\n", surfValid ? "ACTIVE" : "not set (factors = 1.0)", (unsigned)surfCommitCount);
#endif
}

float currentTempC = NAN;  // v6: ย้ายมาประกาศตรงนี้ (ก่อน readAw/readAwAndRaw) เพราะ applyCal() ต้องใช้ค่านี้

// อ่านค่าความชื้นหลายครั้งแล้วเฉลี่ย (oversampling) เพื่อลด noise ของเซนเซอร์
// และตัดค่าที่หลุดผิดปกติ (NaN หรือห่างจากค่ามัธยฐานมากเกินไป) ทิ้ง เพื่อความแม่นยำที่สูงขึ้น
// v7: ลดจำนวนตัวอย่างจาก 9 -> 5 ครั้ง และลดหน่วงระหว่างตัวอย่างจาก 10ms -> 6ms เพื่อให้ตรวจจับ/อัปเดต
// ค่าได้เร็วขึ้นราว 2 เท่า (5 ครั้ง x ~6ms หน่วง ~= 24ms + เวลาอ่านจริงของเซนเซอร์ ยังคงมีการตัด outlier
// ด้วยค่ามัธยฐานเหมือนเดิม จึงยังลด noise ได้ดีอยู่ แม้ตัวอย่างจะน้อยลงกว่าเดิม)
#define AW_OVERSAMPLE_N 5

// ---------- v13: ตัวช่วยอ่าน SHT ให้ทนสัญญาณ I2C หลุดเป็นครั้งคราว ----------
// v17: ฟังก์ชัน/ตัวแปรชุดนี้ตอนนี้ใช้ร่วมกันทั้ง SHT45 (I2C) และ DHT22/DHT11 (ขา DHT_PIN) — ชื่อยังขึ้นต้นด้วย
// "sht" เหมือนเดิมตามของเก่า (ไม่เปลี่ยนชื่อ เพราะมีจุดเรียกใช้อยู่หลายสิบจุดทั้งไฟล์) แต่ภายในจะเช็ค humSensorType
// แล้วไปอ่านจากเซนเซอร์ตัวที่ตรวจพบจริงตอนบูตให้เอง (ดู detectHumiditySensor() ด้านล่าง)
uint8_t shtAddrInUse = 0x44;        // ที่อยู่ I2C ที่เจอตอนบูต (0x44 หรือ 0x45) ใช้ตอน sht.begin() ใหม่หลังล้างบัส (เฉพาะโหมด SHT)
int shtFailStreak = 0;              // อ่านเซนเซอร์ที่ใช้อยู่ไม่ได้เลยติดกันกี่รอบ (SHT หรือ DHT แล้วแต่ humSensorType)
uint32_t shtRecoverCount = 0;       // จำนวนครั้งที่ล้างบัส/เริ่มเซนเซอร์ใหม่ (แสดงใน /info)
unsigned long lastShtReadMs = 0;    // เวลาที่อ่านสำเร็จล่าสุด
float shtTempC = NAN;               // v14: อุณหภูมิตัวชิป SHT (หรือ DHT) ล่าสุด (ใช้ชดเชย %RH เมื่อต่างจากอุณหภูมิตัวอย่าง)
unsigned long lastShtTempMs = 0;
float lastShtRhPct = NAN;           // %RH เฉลี่ยล่าสุดที่อ่านสำเร็จ — handleData() ใช้ต่อได้เลยถ้ายังใหม่ (ไม่อ่านซ้ำซ้อนกับรอบวัด)

// อ่านครั้งเดียว ถ้าได้ NaN ลองอีกครั้งทันทีหลังพัก — SHT: CRC ผิด/NACK จากสัญญาณรบกวน พัก 3 ms พอ
// DHT: โปรโตคอลสายเดียวไวต่อจังหวะเวลามาก ๆ (มักพลาดถ้าถูกเรียกถี่กว่า ~2 วิ) พักนานกว่านั้นก่อนลองซ้ำ
float readShtHumidityRetry() {
  float rh;
  if (humSensorType == HUMSENS_DHT) {
    rh = dht.readHumidity();
    if (isnan(rh)) { delay(50); rh = dht.readHumidity(); }
  } else {
    rh = sht.readHumidity();
    if (isnan(rh)) { delay(3); rh = sht.readHumidity(); }
  }
  return rh;
}

// v17: คู่กับ readShtHumidityRetry() ด้านบน แต่อ่านอุณหภูมิแทน — แยกออกมาจาก refreshShtTempOnly()/readRawAw()
// เดิมที่เรียก sht.readTemperature() ตรง ๆ เพื่อให้ทั้งสองจุดนั้นได้อุณหภูมิจากเซนเซอร์ที่กำลังใช้งานจริงเหมือนกัน
float readShtTemperatureRetry() {
  float st;
  if (humSensorType == HUMSENS_DHT) {
    st = dht.readTemperature();          // DHT22/11 วัดอุณหภูมิได้ในตัวเหมือนกัน (แม่นยำต่ำกว่า DS18B20 แต่พอใช้ชดเชยคร่าวๆ)
    if (isnan(st)) { delay(50); st = dht.readTemperature(); }
  } else {
    st = sht.readTemperature();
    if (isnan(st)) { delay(3); st = sht.readTemperature(); }
  }
  return st;
}

// Phase 3: อ่านเฉพาะอุณหภูมิตัวชิปเซนเซอร์ความชื้น อัปเดต shtTempC/lastShtTempMs อย่างเดียว (ไม่แตะ %RH/currentAw ใด ๆ)
// ใช้ระหว่างขั้น "รอเย็น" (ST_SENSOR_COND) เพื่อให้ gradientDeltaC() มีข้อมูลสดพอจะตัดสินใจข้ามขั้นตอนก่อนเวลาได้จริง
void refreshShtTempOnly() {
  float st = readShtTemperatureRetry();
  if (!isnan(st) && st > -40.0f && st < 125.0f) { shtTempC = st; lastShtTempMs = millis(); }
}

// อ่านไม่ได้ติดกันหลายรอบ -> กู้คืนเซนเซอร์ที่กำลังใช้งานอยู่ แทนที่จะค้าง fault ไปตลอดจนกว่าจะรีบูต
// SHT (I2C): ล้างบัส I2C แล้วสั่ง sht.begin() ใหม่ (soft reset) เหมือนเดิมทุกประการ
// v17 DHT (ขาเดียว): ไม่มีบัสให้ล้าง แค่สั่ง dht.begin() ใหม่ (เคลียร์สถานะภายในไลบรารี) พอ — ไม่ยุ่งกับ I2C/จอ LCD เลย
void shtBusRecover() {
  if (humSensorType == HUMSENS_DHT) {
    Serial.println("[DHT] read failed repeatedly - re-init sensor");
    dht.begin();
  } else {
    Serial.println("[I2C] SHT read failed repeatedly - recovering bus + re-init sensor");
    i2cBusRecover();
    sht.begin(shtAddrInUse);
    sht.heater(sensorHeaterOn);   // soft reset ปิดฮีตเตอร์ในตัวชิป -> คืนสถานะตามที่ระบบสั่งไว้
    lcd.requestReinit();          // สัญญาณรบกวนที่ทำ SHT พัง มักทำให้จอเพี้ยนด้วย
  }
  shtFailStreak = 0;
  shtRecoverCount++;
}

// ---------- v24 (ชุด A): อ่านเซนเซอร์ความแม่นสูง — T/RH จากการวัดครั้งเดียวกัน + ตัวกรอง MAD ----------
// ปัญหาเดิม: อ่านความชื้น 5 ครั้ง (แต่ละครั้ง SHT วัดทั้ง T และ RH แล้วทิ้ง T) แล้วอ่าน T อีกครั้งแยกต่างหาก
//   -> T ที่ใช้ชดเชย Magnus ไม่ได้มาจากจังหวะเดียวกับ RH ที่เฉลี่ยมา (ตอนอุณหภูมิกำลังเปลี่ยน 0.1 °C ≈ 0.6 %RH คลาดได้)
// ใหม่: อ่านเป็นคู่ (T,RH) จากการวัดครั้งเดียว (SHT45: readBoth() ใช้คำสั่ง native 0xFD และตรวจ CRC;
//   DHT: ไลบรารีแคชผลการอ่านรอบเดียวกันให้ทั้งคู่อยู่แล้ว) แล้วกรอง outlier ด้วย MAD (Median Absolute Deviation)
//   แทนเกณฑ์ตายตัว 3 %RH — ปรับตาม noise จริง โดยมีพื้น (SHT_MAD_FLOOR_PCT) กันกรณี MAD=0 แล้วตัดทุกอย่าง
const float SHT_MAD_K = 3.5f;          // ตัดค่าที่ห่างมัธยฐานเกิน K x sigma (sigma = 1.4826 x MAD) — 3.5 เป็นค่ามาตรฐานของ modified z-score
const float SHT_MAD_FLOOR_PCT = 0.10f; // sigma ต่ำสุดที่ยอมใช้ (%RH) ~ noise/ความละเอียดของ SHT45
const float SHT_MAD_CAP_PCT = 3.0f;    // เพดานความกว้างที่ยอมรับ (%RH) กันกรณีข้อมูลกระจายมากจนตัวกรองหลวมเกินไป

// AppState ถูกประกาศไว้แล้วด้านบน แต่ตัวแปร state จริงอยู่ท้ายไฟล์
// จึงต้องประกาศล่วงหน้าก่อนฟังก์ชันที่ใช้งาน
extern AppState state;

// อ่านคู่ (rh %, t °C) จากการวัดเดียว — คืน false ถ้าอ่านไม่ได้/ค่าหลุดช่วง (ลองซ้ำ 1 ครั้งเหมือนเดิม)
bool readShtPair(float& rh, float& t) {
  for (int attempt = 0; attempt < 2; attempt++) {
    float h = NAN, tt = NAN;
    if (humSensorType == HUMSENS_DHT) {
      h = dht.readHumidity();          // DHT: อ่านจริงครั้งเดียว ครั้งถัดไปในหน้าต่างสั้น ๆ ได้ค่าแคชรอบเดียวกัน
      tt = dht.readTemperature();
    } else {
      if (!sht.readBoth(&tt, &h)) { h = NAN; tt = NAN; }
    }
    if (!isnan(h) && !isnan(tt) && h >= 0.0f && h <= 100.0f && tt > -40.0f && tt < 125.0f) { rh = h; t = tt; return true; }
    delay(humSensorType == HUMSENS_DHT ? 50 : 3);
  }
  return false;
}

static float medianOf(float* a, int n) {   // a ถูกเรียงลำดับในที่เดิม (n เล็กมาก)
  for (int i = 1; i < n; i++) { float v = a[i]; int j = i - 1; while (j >= 0 && a[j] > v) { a[j + 1] = a[j]; j--; } a[j + 1] = v; }
  return (n & 1) ? a[n / 2] : 0.5f * (a[n / 2 - 1] + a[n / 2]);
}

float readRawAw() {
  float rhS[AW_OVERSAMPLE_N], tS[AW_OVERSAMPLE_N];
  int n = 0;
  for (int i = 0; i < AW_OVERSAMPLE_N; i++) {
    float rh, t;
    if (readShtPair(rh, t)) { rhS[n] = rh; tS[n] = t; n++; }
    if (i < AW_OVERSAMPLE_N - 1) delay(6);
  }
  if (n == 0) {
    // v-stability: อ่านไม่ได้เลยสักครั้ง -> เซนเซอร์มีปัญหาจริง (สายหลุด/I2C แฮงก์/ขา DHT หลวม)
    sensorFaultSHT = true;
    if (++shtFailStreak >= 3) shtBusRecover();   // v13: พลาด 3 รอบติด -> ล้างบัส/เริ่มเซนเซอร์ใหม่
    return -1;
  }
  sensorFaultSHT = false;
  shtFailStreak = 0;

  // --- ตัวกรอง MAD ---
  float tmp[AW_OVERSAMPLE_N];
  for (int i = 0; i < n; i++) tmp[i] = rhS[i];
  float med = medianOf(tmp, n);
  for (int i = 0; i < n; i++) tmp[i] = fabsf(rhS[i] - med);
  float mad = medianOf(tmp, n);
  float sigma = 1.4826f * mad;
  if (sigma < SHT_MAD_FLOOR_PCT) sigma = SHT_MAD_FLOOR_PCT;
  float lim = SHT_MAD_K * sigma;
  if (lim > SHT_MAD_CAP_PCT) lim = SHT_MAD_CAP_PCT;
  float sumRh = 0, sumT = 0; int cnt = 0;
  for (int i = 0; i < n; i++) {
    if (fabsf(rhS[i] - med) <= lim) { sumRh += rhS[i]; sumT += tS[i]; cnt++; }   // เก็บ T ของตัวอย่างเดียวกับ RH ที่ผ่านตัวกรอง
  }
  float avgRH, avgT;
  if (cnt > 0) { avgRH = sumRh / cnt; avgT = sumT / cnt; }
  else { avgRH = med; avgT = tS[0]; }
  lastShtRhPct = avgRH;
  roomHumidityNow = avgRH;
  if (state == ST_MENU_MAIN && !isnan(ambientRHatBoot)) roomEnvironmentChanged = fabsf(avgRH - ambientRHatBoot) > ROOM_ENV_CHANGE_RH_PCT;
  lastShtReadMs = millis();
  shtTempC = avgT;               // T ชุดเดียวกับ RH — gradientDeltaC() / Magnus ใช้ค่านี้
  lastShtTempMs = millis();
  return avgRH / 100.0;
}

// v17: ตรวจจับตอนบูตว่าตอนนี้ต่อเซนเซอร์ตัวไหนอยู่ — ลอง SHT45 ผ่าน I2C ก่อน (0x44 แล้ว 0x45) เพราะไวและชัวร์กว่า
// ถ้าไม่เจอค่อยลอง DHT22/11 ที่ขา DHT_PIN (ต้องรอ ~2 วิให้เซนเซอร์ตั้งตัวหลังจ่ายไฟ ตามสเปคของ DHT ก่อนอ่านครั้งแรกได้)
// เรียกครั้งเดียวใน setup() แทนที่บล็อก sht.begin() เดิม — ผลลัพธ์เก็บใน humSensorType ให้ทุกฟังก์ชันด้านบนใช้ต่อ
void detectHumiditySensor() {
  bool shtOk = sht.begin(0x44);
  uint8_t shtAddrFound = 0x44;
  if (!shtOk) { shtOk = sht.begin(0x45); shtAddrFound = 0x45; }
  if (shtOk) {
    humSensorType = HUMSENS_SHT;
    shtAddrInUse = shtAddrFound;
    sensorFaultSHT = false;
    Serial.printf("[SENSOR] SHT45 found at I2C 0x%02X — using SHT\n", shtAddrFound);
    return;
  }
  Serial.println("[SENSOR] SHT45 not found on I2C - trying DHT22/11 on GPIO" + String(DHT_PIN) + "...");
  dht.begin();
  delay(2000); // DHT ต้องรอตั้งตัวอย่างน้อย ~1-2 วิหลังไฟเข้าก่อนอ่านค่าแรกได้แม่นยำ
  float rh = dht.readHumidity();
  if (isnan(rh)) { delay(300); rh = dht.readHumidity(); }  // ลองซ้ำอีกครั้งเผื่อจังหวะแรกชนกับ noise
  if (!isnan(rh) && rh >= 0.0f && rh <= 100.0f) {
    humSensorType = HUMSENS_DHT;
    sensorFaultSHT = false;
    Serial.printf("[SENSOR] DHT found on GPIO%d — using DHT\n", DHT_PIN);
  } else {
    humSensorType = HUMSENS_NONE;
    sensorFaultSHT = true;
    Serial.println("[FAULT] No humidity sensor found - checked SHT45 (I2C 0x44/0x45) and DHT (GPIO" + String(DHT_PIN) + ")");
  }
}

// v17: ถ้าตอนบูตหาเซนเซอร์ไม่เจอเลย (humSensorType == HUMSENS_NONE) ลองตรวจซ้ำเป็นระยะระหว่างที่เครื่องว่างอยู่ที่เมนู
// เผื่อผู้ใช้เพิ่งเสียบเซนเซอร์เข้าไปหลังบูตเครื่องไปแล้ว (ถอดสลับ SHT<->DHT โดยไม่อยากรีสตาร์ทบอร์ดทุกครั้ง)
// ไม่ยุ่งกับกรณีที่ตรวจเจอแล้ว (humSensorType != NONE) เพื่อไม่ให้ไปรบกวนจังหวะอ่านค่าปกติที่กำลังทำงานอยู่
unsigned long lastSensorRedetectMs = 0;
const unsigned long SENSOR_REDETECT_MS = 15000UL;
void maybeRedetectHumiditySensor() {
  if (humSensorType != HUMSENS_NONE) return;
  if (state != ST_MENU_MAIN && state != ST_MENU_AW) return;   // ทำเฉพาะตอนเครื่องว่าง ไม่รบกวนรอบวัดที่กำลังทำอยู่
  unsigned long now = millis();
  if (now - lastSensorRedetectMs < SENSOR_REDETECT_MS) return;
  lastSensorRedetectMs = now;
  detectHumiditySensor();
}

// ---------- Phase 1: จดจำความชื้น/อุณหภูมิห้องตอนบูต (ดูคำอธิบายที่จุดประกาศตัวแปรด้านบนไฟล์) ----------
// โหลดค่าที่เคยจับไว้จากการบูตครั้งก่อน ๆ กลับมาก่อน (เผื่อรอบนี้จับไม่สำเร็จ อย่างน้อยยังมีค่าอ้างอิงเก่าให้ใช้)
// เรียกครั้งเดียวใน setup() ก่อน captureAmbientBaseline() เสมอ
void loadAmbientBaseline() {
  prefs.begin("awamb", true);
  bool hasSaved = prefs.isKey("rh");
  if (hasSaved) {
    ambientRHatBoot = prefs.getFloat("rh", NAN);
    ambientTempCatBoot = prefs.getFloat("tempC", NAN);
    ambientBootEpoch = (time_t)prefs.getULong64("ep", 0);
  }
  prefs.end();
  ambientBootValid = false; // ค่าที่เพิ่งโหลดมาเป็นของ "บูตรอบก่อน" ยังไม่ใช่ของรอบนี้ จนกว่า capture จะสำเร็จ
  if (hasSaved) Serial.printf("[AMBIENT] loaded previous baseline: RH=%.1f%% T=%.2fC\n", ambientRHatBoot, ambientTempCatBoot);
  else Serial.println("[AMBIENT] no previous baseline in NVS (first boot ever, or was cleared)");
}

// จับค่าห้อง "ตอนนี้" จริง ๆ — เรียกใน setup() หลัง sht.begin() สำเร็จ และหลัง setSensorHeater(false)
// เท่านั้น (ต้องมั่นใจว่าฮีตเตอร์ในตัวชิปไม่เคยถูกเปิดมาก่อนหน้านี้เลยตั้งแต่ไฟเข้าเครื่อง ไม่งั้นค่าจะไม่ใช่ค่าห้องจริง)
// ไม่แตะ/ไม่เปิดฮีตเตอร์ใด ๆ เอง แค่อ่านค่าเฉลี่ยแบบ oversampling เหมือน readRawAw() แล้วเก็บ
// ถ้าอ่านไม่ได้ (เซนเซอร์มีปัญหา) จะไม่บันทึกทับของเก่า - ปล่อยให้ค่าที่โหลดจาก loadAmbientBaseline() ยังอยู่ต่อไป
void captureAmbientBaseline() {
  if (sensorFaultSHT) {
    Serial.println("[AMBIENT] skip capture - SHT sensor fault at boot");
    return;
  }
  float rh = readRawAw(); // หน่วยเศษส่วน 0-1 พร้อม oversampling/ตัด outlier ในตัวอยู่แล้ว, อัปเดต shtTempC ให้ด้วย
  if (rh < 0 || isnan(shtTempC)) {
    Serial.println("[AMBIENT] skip capture - could not get a valid reading this boot");
    return;
  }
  ambientRHatBoot = rh * 100.0f;
  ambientTempCatBoot = shtTempC;
  ambientBootEpoch = nowEpoch(); // มักจะเป็น 0 ตอนนี้ (ยังไม่ทันซิงก์เวลาจากเว็บ) - ปกติ ไม่ใช่ error
  ambientBootValid = true;
  prefs.begin("awamb", false);
  prefs.putFloat("rh", ambientRHatBoot);
  prefs.putFloat("tempC", ambientTempCatBoot);
  prefs.putULong64("ep", (uint64_t)ambientBootEpoch);
  prefs.end();
  Serial.printf("[AMBIENT] captured this boot: RH=%.1f%% T=%.2fC\n", ambientRHatBoot, ambientTempCatBoot);

  // Phase 2: ถ้ามี DS18B20 ต่ออยู่ ให้รอแปลงค่าเสร็จแล้วอ่านครั้งเดียว (ไม่แตะ currentTempC/dsFailStreak ของ
  // updateDS18B20() เลย - เก็บเป็นตัวอย่างแยกไว้เรียนรู้ offset เท่านั้น) ตอนนี้ห้องยังนิ่งเหมือนตอนอ่าน SHT ด้านบน
  if (!sensorFaultDS18B20) {
    if (waitForDS18B20ConversionOnce(1000UL)) {
      float dsAmbientC = ds18b20.getTempCByIndex(0);
      bool dsValid = (dsAmbientC > -55.0f && dsAmbientC < 125.0f && dsAmbientC != 85.0f);
      if (dsValid) {
        saveLearnedOffsetSample(ambientTempCatBoot - dsAmbientC); // T_SHT - T_DS ตอนห้องนิ่ง ไม่มีฮีตเตอร์ทำงานเลย
      } else {
        Serial.println("[OFFSET] DS18B20 reading not stable yet - skip offset learning this boot");
      }
    } else {
      Serial.println("[OFFSET] DS18B20 conversion timeout - skip offset learning this boot");
    }
    ds18b20.requestTemperatures(); // คืนสถานะให้ updateDS18B20() ในลูปหลักขอแปลงค่ารอบถัดไปได้ตามปกติ
  }
}

// ---------- Phase 2: เรียนรู้ shtOffset จากตัวอย่างห้องนิ่งตอนบูต (ดูคำอธิบายที่จุดประกาศตัวแปรด้านบนไฟล์) ----------
void loadLearnedOffset() {
  prefs.begin("awoff", true);
  bool hasSaved = prefs.isKey("off");
  if (hasSaved) {
    learnedShtDsOffsetC = prefs.getFloat("off", NAN);
    learnedOffsetSampleCount = prefs.getUInt("cnt", 0);
    learnedOffsetSdC = prefs.getFloat("sd", OFFSET_SD_SEED_C);   // v25: บูตเก่าที่ยังไม่มี sd ใน NVS -> ใช้ค่าตั้งต้นแบบระวัง
    if (isnan(learnedOffsetSdC) || learnedOffsetSdC < 0.0f) learnedOffsetSdC = OFFSET_SD_SEED_C;
  }
  prefs.end();
  learnedOffsetTrusted = hasSaved && learnedOffsetSampleCount >= OFFSET_LEARN_MIN_SAMPLES_TRUST && !isnan(learnedShtDsOffsetC);
  if (hasSaved) {
    Serial.printf("[OFFSET] loaded learned offset: %.2fC from %u samples (trusted=%s)\n",
                  learnedShtDsOffsetC, (unsigned)learnedOffsetSampleCount, learnedOffsetTrusted ? "yes" : "no");
  } else {
    Serial.println("[OFFSET] no learned offset in NVS yet - using fixed SHT_MINUS_DS_OFFSET_C until enough boots collected");
  }
}

// รับตัวอย่าง offset ใหม่ 1 ตัวจากบูตนี้ (T_SHT - T_DS วัดพร้อมกันตอนห้องนิ่ง) แล้วผสมเข้ากับค่าที่เรียนรู้สะสมด้วย EMA
// ตัวอย่างแรกสุด (ยังไม่มีค่าก่อนหน้า) ใช้เป็นค่าตั้งต้นตรง ๆ - ตัวอย่างถัดไปที่กระโดดเกิน OFFSET_LEARN_MAX_JUMP_C
// ถือว่าห้องยังไม่นิ่งจริง/มีสัญญาณรบกวน จะถูกทิ้ง ไม่ให้มาดึงค่าเฉลี่ยเพี้ยน
void saveLearnedOffsetSample(float sampleOffsetC) {
  if (isnan(sampleOffsetC)) return;
  // v25: เกณฑ์ทิ้งตัวอย่างแคบลงเมื่อ offset เชื่อถือได้แล้ว (ตัวอย่างที่ห่างมาก = ห้อง/ชิปยังไม่นิ่งตอนบูต ไม่ใช่ offset จริง)
  float maxJump = learnedOffsetTrusted ? OFFSET_LEARN_MAX_JUMP_TRUSTED_C : OFFSET_LEARN_MAX_JUMP_C;
  if (!isnan(learnedShtDsOffsetC) && fabs(sampleOffsetC - learnedShtDsOffsetC) > maxJump) {
    Serial.printf("[OFFSET] sample %.2fC rejected - too far from learned %.2fC (room likely not settled this boot)\n",
                  sampleOffsetC, learnedShtDsOffsetC);
    return;
  }
  if (isnan(learnedShtDsOffsetC)) {
    learnedShtDsOffsetC = sampleOffsetC;          // ตัวอย่างแรก = ค่าตั้งต้น
    learnedOffsetSdC = OFFSET_SD_SEED_C;
  } else {
    // v25: น้ำหนักตัวอย่างใหม่ = max(ALPHA, 1/(n+1)) — ช่วงแรกเป็นค่าเฉลี่ยถ่วงเท่ากัน (ประสิทธิภาพทางสถิติสูงสุดเมื่อข้อมูลน้อย)
    // แล้วค่อยเปลี่ยนเป็น EMA ตามเดิมเมื่อสะสมพอ (ตาม drift ของเซนเซอร์ระยะยาวได้)
    float alpha = 1.0f / ((float)learnedOffsetSampleCount + 1.0f);
    if (alpha < OFFSET_LEARN_ALPHA) alpha = OFFSET_LEARN_ALPHA;
    float dev = sampleOffsetC - learnedShtDsOffsetC;
    learnedShtDsOffsetC += alpha * dev;
    // ความแปรปรวนแบบ exponentially-weighted (West 1979): var' = (1-a)(var + a*dev^2)
    float sd0 = isnan(learnedOffsetSdC) ? OFFSET_SD_SEED_C : learnedOffsetSdC;
    float var = (1.0f - alpha) * (sd0 * sd0 + alpha * dev * dev);
    learnedOffsetSdC = sqrtf(var);
  }
  learnedOffsetSampleCount++;
  learnedOffsetTrusted = learnedOffsetSampleCount >= OFFSET_LEARN_MIN_SAMPLES_TRUST;
  prefs.begin("awoff", false);
  prefs.putFloat("off", learnedShtDsOffsetC);
  prefs.putFloat("sd", learnedOffsetSdC);
  prefs.putUInt("cnt", learnedOffsetSampleCount);
  prefs.end();
  Serial.printf("[OFFSET] sample %.2fC accepted -> learned=%.2fC sd=%.2fC (n=%u, trusted=%s, deadband=%.2fC)\n",
                sampleOffsetC, learnedShtDsOffsetC, learnedOffsetSdC, (unsigned)learnedOffsetSampleCount,
                learnedOffsetTrusted ? "yes" : "no", gradientDeadbandC());
}

// ค่า offset ที่ควรใช้จริงในการคำนวณ gradientDeltaC() - ใช้ค่าที่เรียนรู้แล้วถ้าเชื่อถือได้ (ตัวอย่างพอ)
// ไม่งั้น fallback ไปค่าคงที่ SHT_MINUS_DS_OFFSET_C เดิมที่ตั้งมือ (พฤติกรรมเดิมทุกประการสำหรับเครื่องที่ยังไม่มีประวัติ)
float getEffectiveShtDsOffsetC() {
  return learnedOffsetTrusted ? learnedShtDsOffsetC : SHT_MINUS_DS_OFFSET_C;
}

// รอ DS18B20 แปลงค่าเสร็จครั้งเดียว (ใช้เฉพาะตอนบูต ไม่ใช้ใน loop() หลักเพราะจะบล็อกโปรแกรม) คืน false ถ้าเกิน timeout
bool waitForDS18B20ConversionOnce(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (!ds18b20.isConversionComplete()) {
    if (millis() - start > timeoutMs) return false;
    delay(10);
    esp_task_wdt_reset();
  }
  return true;
}

float currentAw = 0.5;
float currentRawAw = 0.5;  // ค่าดิบ (RH เศษส่วน 0-1) ก่อนผ่านสมการคาลิเบรต ใช้เป็นเกณฑ์ตัดสินความนิ่ง
// v29: ทางคำนวณ aw ที่ "แสดง" ทางเดียว — เดิมจอเครื่อง = ตารางล้วน / เว็บ = ตาราง+offset (และเว็บโพลเป็นตัวเดินสถานะ)
// ตอนนี้ลูปวัดบนเครื่องเป็นตัวเดินสถานะเพียงตัวเดียว เว็บอ่านค่าที่แคชไว้ตัวเดียวกัน
extern unsigned long measureStartMs;
static bool substanceMeasuringNow() {
  return (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE);
}
static void substanceAdvance(float raw) {
  static bool prevLid = false;
  bool lidOpenNow = updateLidEventDetector(raw, millis());
  if (lidOpenNow) {
    resetSubstanceGuessWindow();       // กำลังเปิดฝา: ข้อมูลยังไม่น่าเชื่อ เริ่มนับใหม่ (แสดงค่าดิบระหว่างนี้)
  } else {
    if (prevLid) resetStabilityWindow();   // เปิดฝาเสร็จ (true->false) -> ตัวตรวจนิ่งเริ่มใหม่ด้วย (เหมือนเดิมที่เคยทำใน /data)
    updateSubstanceGuess(raw, measureStartMs);
  }
  prevLid = lidOpenNow;
}
float awShownFromRaw(float raw, float tempC) {
  float awCal = applyCal(raw, tempC);
#if AW_RAW_MODE
  return awCal;
#else
  // v-raw-idle: ยังไม่ได้กดวัด (เมนู/หน้าสด) แสดงค่าดิบ RH/100 ; คาลิเบรตเฉพาะตอนวัด (Measure/Predict/Compare) หรือโหมดคาลิเบตอัตโนมัติ
  if (!substanceMeasuringNow()) return autoCalActive() ? awCal : constrain(raw, 0.0f, 1.0f);
  substanceAdvance(raw);
  return applySubstanceGuessBias(awCal, raw);
#endif
}

float readAw() {
  float raw = readRawAw();
  if (raw < 0) return currentAw;
  return applyCal(raw, currentTempC);
}
// อ่านค่าทั้งค่าที่คาลิเบรตแล้ว (คืนค่ากลับ) และค่าดิบ (ผ่าน rawOut) ในการอ่านครั้งเดียวกัน
// เพื่อให้ตรรกะ "นิ่งหรือยัง" ใช้ค่าดิบเป็นเกณฑ์ ไม่ใช่ค่าที่ผ่านสมการคาลิเบรตซึ่งอาจถูกขยาย/บีบความแปรปรวน
float readAwAndRaw(float& rawOut) {
  float raw = readRawAw();
  if (raw < 0) {
    rawOut = currentRawAw;
    return currentAw;
  }
  currentRawAw = raw;
  rawOut = raw;
  advEkfFeed(raw);   // v29: EKF ผู้สังเกตการณ์
  return awShownFromRaw(raw, currentTempC);   // v29: ทางเดียวกับที่เว็บ/ค่าที่ล็อกใช้
}

unsigned long lastTempReadMs = 0;
const unsigned long TEMP_READ_INTERVAL_MS = 1000;

unsigned long dashboardStartTime = 0; // ใช้คำนวณ "เวลาที่ผ่านไป (นาที)" สำหรับกราฟบนเว็บ

void beginDS18B20() {
  ds18b20.begin();
  ds18b20.setResolution(12);   // v25: ระบุ 12-bit (0.0625 °C) ชัดเจน ไม่พึ่งค่าที่ค้างใน scratchpad ของชิป (แปลง ~750 ms < รอบอ่าน 1000 ms)
  ds18b20.setWaitForConversion(false);
  ds18b20.requestTemperatures();
  lastTempReadMs = millis();
}

int dsFailStreak = 0;              // v13: อ่านไม่ได้ติดกันกี่ครั้ง
int dsSpikeStreak = 0;             // v13: ค่ากระโดดที่ถูกข้ามติดกันกี่ครั้ง
unsigned long dsLastRecoverMs = 0;

void updateDS18B20() {
  if (millis() - lastTempReadMs < TEMP_READ_INTERVAL_MS) return;
  lastTempReadMs = millis();
  float t = ds18b20.getTempCByIndex(0);
  // DallasTemperature คืนค่า DEVICE_DISCONNECTED_C (-127) เมื่อสายหลุด/ไม่พบอุปกรณ์บนบัส 1-Wire
  bool valid = (t > -55.0f && t < 125.0f);
  // v13: DS18B20 คืนค่า 85.0 °C ทุกครั้งที่เพิ่งรีเซ็ต/แปลงค่ายังไม่เสร็จ (เกิดตอนไฟเลี้ยงตกชั่วขณะ) — ค่านี้ทำให้ PID
  // เห็นว่า "ร้อนจัด" แล้วสั่งเทลเทียร์เต็มกำลัง และค่า aw ที่ชดเชยอุณหภูมิเพี้ยน จึงตัดทิ้งถ้าไม่ได้ต่อเนื่องมาจากช่วงร้อนจริง
  if (valid && t == 85.0f && (isnan(currentTempC) || fabs(currentTempC - 85.0f) > 10.0f)) valid = false;
  // v13: ตัวเซนเซอร์มีมวลความร้อน อุณหภูมิเปลี่ยนเกิน TEMP_SPIKE_C ใน 1 วินาทีเป็นไปไม่ได้จริง -> เป็นสัญญาณรบกวน ข้ามค่านั้น
  // (แต่ถ้าค่าใหม่คงอยู่ติดกัน 3 ครั้ง ถือว่าเปลี่ยนจริง ยอมรับ — และถ้าเพิ่งกู้จาก fault ให้รับค่าใหม่ทันที เพราะค่าเก่าค้างเป็นค่าเก่าแล้ว)
  if (valid && !isnan(currentTempC) && !sensorFaultDS18B20 && fabs(t - currentTempC) > TEMP_SPIKE_C && dsSpikeStreak < 3) {
    dsSpikeStreak++;
    ds18b20.requestTemperatures();
    return;
  }
  if (valid) {
    currentTempC = t;
    advKfUpdate(t);   // v29: Kalman filter ของ DS18B20
    dsFailStreak = 0;
    dsSpikeStreak = 0;
    sensorFaultDS18B20 = false;
  } else {
    // v-stability: ประกาศ fault ต่อเมื่ออ่านพลาดติดกัน 3 ครั้ง (เดิมพลาดครั้งเดียวก็ขึ้น fault ทำให้ธงกระพริบ)
    if (++dsFailStreak >= 3) { sensorFaultDS18B20 = true; advKfReset(); }
    // พลาดต่อเนื่อง 5 ครั้ง -> ลองสแกนบัส 1-Wire ใหม่ (ทุก 5 วินาที) เผื่อสายหลวมชั่วคราวแล้วกลับมาต่อติด
    if (dsFailStreak >= 5 && millis() - dsLastRecoverMs > 5000UL) {
      dsLastRecoverMs = millis();
      ds18b20.begin();
      ds18b20.setResolution(12);
      ds18b20.setWaitForConversion(false);
    }
  }
  ds18b20.requestTemperatures();
}

float TARGET_TEMP_C = 25.0;  // v-web-ctrl: เดิมเป็น const — เอา const ออกเพื่อให้โหมดคาลิเบตอัตโนมัติของแอดมิน (ดู updateAutoCalMode()) ปรับอุณหภูมิเป้าหมายเป็นรอบ ๆ ได้ (25/25/25/20/17 °C)
const float TARGET_TEMP_C_DEFAULT = 25.0; // ค่าเริ่มต้นปกติ ใช้ตอนไม่ได้อยู่ในโหมดคาลิเบตอัตโนมัติ / ตอนยกเลิก
const unsigned long WARMUP_HOLD_MS = 30000;

// v-cal-cancel: ต้อง forward-declare ตัวนี้ก่อน (นิยามจริงอยู่ท้ายไฟล์ ใกล้ AppState state) เพราะ cancelAutoCalMode()
// ด้านล่างต้องสั่งบังคับให้ loop() วาดหน้าเมนูใหม่ทันทีที่ยกเลิกคาลิเบตจากเว็บ (ดูคำอธิบายในตัวฟังก์ชัน)
extern AppState prevDrawnState;

float pid_Kp = 40.0;
float pid_Ki = 0.1;
float pid_Kd = 10.0;
float pid_Integral = 0;
float pid_LastError = 0;
unsigned long pid_LastTime = 0;
int peltierOutputPWM = 0;
bool peltierOn = false;
// v13: จำกัดอัตราเร่งกำลังเทลเทียร์+พัดลม (slew) และหน่วงไม่ให้เริ่มทำงานช่วงเพิ่งบูต — ลดกระแสกระชากที่ทำให้ไฟตก/บอร์ดรีบูต
float peltierAppliedF = 0;                // ค่า PWM ที่ส่งออกจริง (ทศนิยม เพื่อสะสมการเร่งทีละน้อยได้)
unsigned long peltierLastWriteMs = 0;
unsigned long peltierStartAllowedMs = 0;  // เทลเทียร์ห้ามทำงานก่อนถึงเวลานี้ (ตั้งใน setup() หลัง Wi-Fi ขึ้น)
// v14: เทลเทียร์ของเครื่องนี้ "ทำความเย็นอย่างเดียว" (ขับ PWM ทางเดียว) — ถ้าห้องแอร์เย็นกว่าเป้าหมาย (25 °C) เครื่องจะทำความร้อน
// ให้ถึงเป้าไม่ได้ อุณหภูมิห้องวัดจึงลอยตามแอร์ (เปิด-ตัดของคอมเพรสเซอร์) และค่า aw ของตัวอย่างเปลี่ยนตามอุณหภูมิ
bool coldRoomWarn = false;
unsigned long coldRoomSinceMs = 0;

// ---------------------------------------------------------------------------------------------
// v18: "คงที่" ต้องอิง PWM เทลเทียร์ด้วย
// ปัญหาเดิม: PID เทลเทียร์ทำให้อุณหภูมิแกว่งขึ้น-ลงเล็กน้อยรอบเป้าหมายตลอดเวลา -> aw ที่วัดได้ขึ้น-ลงหน่อย ๆ ตามอุณหภูมิ
// เกณฑ์ช่วงกว้าง aw (0.0008) + ช่วงกว้างอุณหภูมิ (0.4 °C) ที่ตัดสินจากค่า aw/อุณหภูมิอย่างเดียวจึงแทบไม่เคยนับว่า "คงที่"
// (ทั้งโหมดวัดปกติและโหมดคาลิเบตอัตโนมัติ) ทั้งที่ลูปควบคุมนิ่งแล้ว
// หลักการใหม่: ถ้า PWM (เฉลี่ยรายบล็อก 5 วิ) นิ่งตลอดหน้าต่าง = ลูปควบคุมเข้าสมดุลแล้ว -> ผ่อนเกณฑ์ช่วงกว้าง aw / ความชัน /
// ช่วงกว้างอุณหภูมิ ตามตัวคูณด้านล่าง (ค่าที่ขึ้น-ลงหน่อยเป็นแค่ ripple ของการควบคุม) แต่ถ้า PWM ยังไม่นิ่ง (ยังไล่อุณหภูมิ /
// แกว่งแรง / เต็มกำลัง / เทลเทียร์ปิดอยู่) ใช้เกณฑ์เข้มเดิมทุกประการ
// ---------------------------------------------------------------------------------------------
const float STAB_PWM_RANGE_TOL_PCT = 8.0f;    // PWM (% ของ 255) เฉลี่ยรายบล็อกแกว่งได้ไม่เกินนี้ในหน้าต่างเดียวกัน = "PWM นิ่ง"
const float STAB_PWM_MIN_PCT = 1.0f;          // PWM เฉลี่ยต่ำกว่านี้ = เทลเทียร์ปิดอยู่ (ไม่ได้คุมอุณหภูมิ) ไม่นับว่า PWM นิ่ง
const float STAB_PWM_MAX_PCT = 98.0f;         // PWM เฉลี่ยสูงกว่านี้ = เปิดเต็มกำลังแต่ยังไม่ถึงเป้า (ยังไล่อยู่) ไม่นับว่า PWM นิ่ง
const float STAB_PWM_AW_TOL_MULT = 2.5f;      // PWM นิ่ง: ผ่อนช่วงกว้าง aw ที่ยอมรับ (0.0008 -> 0.0020)
const float STAB_PWM_SLOPE_MULT = 1.5f;       // PWM นิ่ง: ผ่อนความชัน aw/นาที ที่ยอมรับ (ripple ทำให้ความชันที่ประมาณได้สุ่มคลาด)
const float STAB_PWM_TEMP_TOL_MULT = 2.0f;    // PWM นิ่ง: ผ่อนช่วงกว้างอุณหภูมิที่ยอมรับ (0.4 -> 0.8 °C)
// v19: ripple ของ aw ที่เกิดจากอุณหภูมิแกว่งตาม PWM เป็นค่าทางกายภาพ (ไม่ขึ้นกับว่าผู้ใช้ตั้ง SAVE_PROMPT_TOL เข้ม/หลวมแค่ไหน)
// ตัวคูณอย่างเดียวจึงพัง: ถ้าปรับ SAVE_PROMPT_TOL ลงเป็น 0.0005 เกณฑ์ตอน PWM นิ่งจะเหลือ 0.00125 < ripple จริง แล้วไม่เคยนิ่งอีก
// -> เกณฑ์ช่วงกว้าง aw ตอน PWM นิ่ง = max(tol x MULT, tol + STAB_PWM_AW_RIPPLE)  (ค่าเริ่มต้น 0.0008 ได้ 0.0020 เท่าเดิมเป๊ะ)
// ค่าเริ่มต้นตั้งจากเกณฑ์ v18 (0.0020 - 0.0008) ยังไม่ได้วัดจริง: ดู stabRangeAw ตอนไฟเขียวขณะ pwmSteady=true แล้วปรับให้พอดีเครื่อง
const float STAB_PWM_AW_RIPPLE = 0.0012f;     // aw peak-to-peak ที่ยอมให้เป็น ripple จากอุณหภูมิ/PWM
bool stabPwmSteadyNow = false;                // โหมดวัด/เปรียบเทียบ: PWM นิ่งตลอดหน้าต่าง 60 วิล่าสุดหรือไม่ (ส่งให้เว็บใน /data)
bool acalPwmSteadyWin = false;                // โหมดคาลิเบตอัตโนมัติ: เช่นเดียวกัน (ตั้งค่าใน computeAutoCalHoldPhase())

// =====================================================================================
// v-web-ctrl: โหมดคาลิเบตอัตโนมัติสำหรับแอดมิน — วัด 5 รอบ รอบละ 25 นาที
//   รอบ 1-3 = 25 °C, รอบ 4 = 20 °C, รอบ 5 = 17 °C (เทลเทียร์ทำความเย็นได้อย่างเดียวอยู่แล้ว ลดอุณหภูมิลงได้ทุกรอบ)
//   แต่ละรอบ: รอให้ห้อง/ตัวอย่างเย็นถึงอุณหภูมิเป้าหมาย (±AUTOCAL_TEMP_TOL_C) นิ่งต่อเนื่อง AUTOCAL_SETTLE_MS
//             ก่อน แล้วค่อยเริ่มจับเวลา 25 นาที เก็บค่า raw/aw/temp เฉลี่ยทั้งรอบไว้เป็นผลลัพธ์คาลิเบตของรอบนั้น
//   ควบคุมจากเว็บเท่านั้น (แผงแอดมิน) ดูสถานะสดผ่าน GET /admin/calmode/status
// =====================================================================================
// v-shared-graph: ย้าย MAX_POINTS/values[]/tempValues[]/numPoints มาไว้ก่อนส่วนคาลิเบตอัตโนมัติ (เดิมอยู่ท้ายไฟล์
// ใกล้ runMeasureAWTick()) เพราะตอนนี้ updateAutoCalMode()/updateAutoCalBoardGraph() ด้านล่างต้องอ้างถึงตัวแปร
// ชุดนี้ตรงๆ (ป้อนกราฟคาลิเบตเข้าบัฟเฟอร์เดียวกับกราฟตรวจวัดปกติ) — ต้องประกาศตัวแปรก่อนจุดที่ใช้งานเสมอ (C++
// ไม่เหมือนฟังก์ชันที่ Arduino IDE auto-generate prototype ให้ล่วงหน้า) นิยามจริงยังอยู่ที่เดิม ไม่ได้ซ้ำซ้อน
#define MAX_POINTS 220
float values[MAX_POINTS];
float tempValues[MAX_POINTS]; // อุณหภูมิคู่กับแต่ละจุดใน values[] สำหรับวาดเส้นประบนจอ TFT
int numPoints = 0;

// v16 (ขั้นที่ 2): คาลิเบตอัตโนมัติ 10 รอบ = 5 คู่ (วัด -> ทดสอบ) ที่อุณหภูมิ 25 / 25 / 25 / 20 / 19 °C
//   รอบคี่ (1,3,5,7,9) = "วัด"  : อ่านค่าดิบตอนนิ่งของสารละลายมาตรฐาน -> ตั้ง/ปรับ "จุดคาลิเบรตชั่วคราว" (raw -> aw อ้างอิง)
//   รอบคู่ (2,4,6,8,10) = "ทดสอบ": ใช้จุดชั่วคราวนั้นวัดซ้ำ ถ้า aw ที่ได้ห่างเป้าไม่เกิน AUTOCAL_VERIFY_TOL_AW = ผ่าน (เฉลี่ยจุดเพิ่ม)
//                                  ถ้าไม่ผ่าน = "แก้ใหม่" (ปรับจุดให้ตรงค่าที่อ่านล่าสุด) แล้วไปรอบถัดไป
//   จบแต่ละรอบเมื่อค่า "นิ่ง" (ตัวตรวจเดียวกับโหมดวัดปกติ: ไฟเขียว) ไม่ใช่รอ 25 นาทีตายตัว — ถ้าไม่นิ่งภายใน AUTOCAL_MAX_ROUND_MS ตัดจบรอบ (ติดธง timeout)
//   จุดที่ได้เก็บแยกตามอุณหภูมิ 3 ช่อง: [0]=25°C (รอบ 1-6), [1]=20°C (รอบ 7-8), [2]=19°C (รอบ 9-10) — ใช้ต่อในขั้นชดเชยตามอุณหภูมิ
#define AUTOCAL_ROUNDS 10
const float AUTOCAL_TARGET_C[AUTOCAL_ROUNDS] = { 25.0f, 25.0f, 25.0f, 25.0f, 25.0f, 25.0f, 20.0f, 20.0f, 19.0f, 19.0f };
#define AUTOCAL_SLOTS 3
const unsigned long AUTOCAL_MAX_ROUND_MS = 30UL * 60UL * 1000UL;   // เพดานเวลาต่อรอบ 30 นาที (ปกติจบเร็วกว่ามากเมื่อค่านิ่ง)
const float AUTOCAL_TEMP_TOL_C = 0.3f;                          // ถือว่า "ถึงเป้าหมาย" เมื่อห่างจากเป้าไม่เกินนี้
const unsigned long AUTOCAL_SETTLE_MS = 60UL * 1000UL;          // เปลี่ยนอุณหภูมิเป้าหมาย: ต้องอยู่ในช่วงยอมรับได้ต่อเนื่องเท่านี้ก่อนเริ่มวัด
const unsigned long AUTOCAL_SETTLE_SAME_MS = 10UL * 1000UL;     // อุณหภูมิเป้าหมายเดิม (รอบคู่/คี่ติดกัน): รอสั้น ๆ พอ
const int AUTOCAL_WIN_SAMPLES = 240;                            // v23: เดิม 60 (=STAB_WINDOW_MS เดิม 60 วิ) ตามขึ้นเป็น 240 ให้ตรงกับ STAB_WINDOW_MS ใหม่ (4 นาที) — ดู static_assert คู่กันท้ายไฟล์
const float AUTOCAL_TEMP_WIN_TOL_C = 0.4f;                      // อุณหภูมิในหน้าต่างล็อกต้องแกว่งไม่เกินนี้ (เท่า STAB_TEMP_RANGE_TOL_C)
const float AUTOCAL_VERIFY_TOL_AW = 0.0015f;                    // รอบทดสอบ: |aw - ค่าอ้างอิง| ไม่เกินนี้ = ผ่าน
// v19: ค่าที่ล็อกคือค่าเฉลี่ยในหน้าต่างที่ aw แกว่งตาม PWM (ripple) — เฉลี่ยไม่ครบรอบ PID พอดีจะเอียงได้อีกเล็กน้อย
// รอบทดสอบที่ PWM นิ่งจึงยอมให้คลาดเพิ่มได้อีกเท่านี้ (ไม่งั้นรอบ "แก้ใหม่" ถูกสั่งทั้งที่ความคลาดเป็นแค่ ripple) — ค่าประมาณ ปรับตามเครื่องจริง
const float AUTOCAL_VERIFY_PWM_EXTRA_AW = 0.0005f;

enum AutoCalPhase { ACAL_IDLE, ACAL_COOLING, ACAL_MEASURING, ACAL_DONE };

struct AutoCalRoundResult {
  float targetC;
  float avgRawFrac;   // RAW เฉลี่ยในหน้าต่างที่ล็อก (นิ่ง)
  float avgAw;        // aw เฉลี่ย ณ หน้าต่างเดียวกัน (ผ่านจุดชั่วคราวที่ใช้อยู่ตอนนั้น)
  float avgTempC;     // อุณหภูมิตัวอย่างเฉลี่ย (DS18B20) ในหน้าต่างเดียวกัน
  int   sampleCount;  // 0 = รอบนี้ยังไม่เสร็จ
  bool  isVerify;     // true = รอบทดสอบ, false = รอบวัด
  bool  passed;       // รอบทดสอบ: aw ตรงเป้าในเกณฑ์ / รอบวัด: true เสมอ
  bool  fixed;        // รอบทดสอบที่ไม่ผ่านแล้วถูก "แก้ใหม่" (ปรับจุดตามค่าที่อ่านล่าสุด)
  bool  timedOut;     // จบเพราะเกินเวลาสูงสุด ไม่ใช่เพราะนิ่ง (ค่าเชื่อถือได้น้อยกว่า)
  float errAw;        // aw - ค่าอ้างอิง (NAN = ไม่มี)
  float pointRaw;     // ค่า raw ของจุดคาลิเบรตชั่วคราว (ช่องอุณหภูมินี้) หลังจบรอบ
  unsigned long durSec; // เวลาที่ใช้วัดรอบนี้ (วินาที)
};

AutoCalPhase autoCalPhase = ACAL_IDLE;
bool autoCalActive() { return autoCalPhase != ACAL_IDLE; }
int autoCalRound = 0;                 // ดัชนี 0-based ของรอบที่กำลังทำ/กำลังจะทำ
unsigned long autoCalPhaseStartMs = 0;
unsigned long autoCalSettleSinceMs = 0;
int autoCalSampleCount = 0;
unsigned long autoCalLastSampleMs = 0;
AutoCalRoundResult autoCalResults[AUTOCAL_ROUNDS];
float acalPointRaw[AUTOCAL_SLOTS];    // จุดคาลิเบรตชั่วคราว raw ต่อช่องอุณหภูมิ (NAN = ยังไม่มี)
int   acalPointN[AUTOCAL_SLOTS];      // จำนวนค่าที่เฉลี่ยรวมอยู่ในจุดนั้น
float acalSlotTempC[AUTOCAL_SLOTS];   // v27: อุณหภูมิตัวอย่างเฉลี่ย (ถ่วงน้ำหนักเท่ากับจุด raw) ของแต่ละช่อง — ใช้สร้างพื้นผิว 2 มิติ
float acalSlotGf[AUTOCAL_SLOTS];      // v27: gradientFactor() เฉลี่ย ณ ตอนล็อกจุดของช่องนั้น (ส่วนต่างอุณหภูมิชิป-ตัวอย่างที่ใช้ตอนนั้น)

// v-acal-graph: ค่าเป้าหมาย (aw อ้างอิงของสารละลายมาตรฐาน) ที่แอดมินต้องกรอกในขั้นตอนแรกก่อนเริ่ม (ดู handleAdminCalStart)
// ใช้เทียบกับ aw เฉลี่ยที่วัดได้ของแต่ละรอบ (ดู errPct ใน handleAdminCalStatus) เพื่อให้คาลิเบตได้ถูกต้องตรงกับมาตรฐานจริง
float autoCalRefAw = NAN;   // NAN = ยังไม่ได้กรอก/ยังไม่เคยเริ่ม

// v-acal-graph: บัฟเฟอร์กราฟสด (aw คาลิเบรตแล้ว) ของรอบที่กำลังวัดอยู่ — ใช้ป้อนกราฟฝั่งเว็บ (ผ่าน /calmode/status)
// เท่านั้น ส่วนกราฟบนจอบอร์ดเปลี่ยนมาใช้ values[]/tempValues[]/pushValue() ชุดเดียวกับหน้าตรวจวัดปกติแล้ว (v-shared-graph)
// ยังคงบัฟเฟอร์นี้ไว้แยกเพราะเว็บต้องได้ค่าทุกจุดแบบไม่ downsample ไปคำนวณ CSV/กราฟทำนายเอง
#define AUTOCAL_GRAPH_CAP 520  // v23: เดิม 300 (=5 นาที) แคบกว่า STAB_SLOPE_WINDOW_MS ใหม่ (8 นาที = 480 ตัวอย่าง) เผื่อไว้ 520 — ดู static_assert คู่กันด้านล่างไฟล์
float autoCalGraphBuf[AUTOCAL_GRAPH_CAP];
int autoCalGraphCount = 0;
// v-shared-graph: true = วาดกรอบ/แกน/หัวข้อของกราฟบนจอบอร์ดไปแล้วสำหรับรอบปัจจุบัน (ตั้งกลับเป็น false ทุกครั้งที่
// เริ่มรอบใหม่ ดู startAutoCalMode()/updateAutoCalMode()) กันไม่ให้วาดกรอบซ้ำทุก 1 วินาทีโดยไม่จำเป็น
bool acalBoardFrameDrawn = false;

// v16: บัฟเฟอร์ค่าดิบ/อุณหภูมิคู่กับ autoCalGraphBuf (1 ตัวอย่าง/วิ, เลื่อนเมื่อเต็ม) ใช้เฉลี่ยค่าที่ล็อกในหน้าต่างนิ่ง
int acalPrevHoldPhase = 0;   // v16: จำสีล่าสุด ใช้ฮิสเทอรีซิสเขียว (กันกระพริบเขียว<->เหลือง) — ใช้ใน computeAutoCalHoldPhase()
float autoCalRawBuf[AUTOCAL_GRAPH_CAP];
float autoCalTempBuf[AUTOCAL_GRAPH_CAP];
float autoCalPwmBuf[AUTOCAL_GRAPH_CAP];   // v18: PWM (%) ที่ส่งออกจริง ณ จังหวะสุ่มแต่ละจุด (คู่กับ autoCalGraphBuf) ใช้ตัดสิน "PWM นิ่ง"
int computeAutoCalHoldPhase();   // นิยามท้ายไฟล์ (ตัวตัดสินนิ่งเดียวกับโหมดวัดปกติ)

// ช่องอุณหภูมิของรอบ r: 0 = 25°C (รอบ 0-5), 1 = 20°C (รอบ 6-7), 2 = 19°C (รอบ 8-9)
int autoCalSlotOf(int r) { return (r < 6) ? 0 : ((r < 8) ? 1 : 2); }
bool autoCalIsVerifyRound(int r) { return (r % 2) == 1; }

// จุดคาลิเบรตชั่วคราวที่ "ใช้อยู่" ในรอบปัจจุบัน — รอบวัดที่อุณหภูมิใหม่ (20/19°C) ยังใช้จุด 25°C เพื่อให้เห็นว่าอุณหภูมิทำให้ค่าเลื่อนเท่าไร
float acalActivePointRaw() {
  int slot = autoCalSlotOf(autoCalRound);
  if (slot > 0 && !autoCalIsVerifyRound(autoCalRound)) return acalPointRaw[0];
  return acalPointRaw[slot];
}
// ความชันเฉพาะที่ของสมการคาลิเบรตรอบจุด P (aw ต่อ raw) — ใช้สร้างจุดชั่วคราว: aw = ref + slope*(raw - P)
float acalSlopeAt(float P, float tempC) {
  float sl = (applyCal(P + 0.01f, tempC) - applyCal(P - 0.01f, tempC)) / 0.02f;
  return (sl > 0.2f && sl < 5.0f) ? sl : 1.0f;
}
// aw ของรอบคาลิเบต: มีจุดชั่วคราวแล้ว -> ผ่านจุดนั้น (ให้ตรงค่าอ้างอิงที่จุด) ยังไม่มี -> สมการคาลิเบรตปกติ
float acalMapAw(float raw, float tempC) {
  float P = acalActivePointRaw();
  if (isnan(P) || isnan(autoCalRefAw)) return applyCal(raw, tempC);
  return constrain(autoCalRefAw + acalSlopeAt(P, tempC) * (raw - P), 0.0f, 1.0f);
}

// เริ่มโหมดคาลิเบตอัตโนมัติใหม่ทั้งชุด (เรียกจาก handleAdminCalStart หลังผ่านเงื่อนไขครบแล้วเท่านั้น)
void startAutoCalMode() {
  autoCalRound = 0;
  autoCalPhase = ACAL_COOLING;
  autoCalPhaseStartMs = millis();
  autoCalSettleSinceMs = 0;
  autoCalSampleCount = 0;
  autoCalGraphCount = 0;
  // v-shared-graph: ล้างบัฟเฟอร์กราฟตัวเดียวกับที่ใช้วาดหน้าตรวจวัดปกติ (values[]/tempValues[]/numPoints) ด้วย
  numPoints = 0;
  resetGraphFilter();
  acalBoardFrameDrawn = false;
  for (int i = 0; i < AUTOCAL_ROUNDS; i++) autoCalResults[i].sampleCount = 0;
  for (int i = 0; i < AUTOCAL_SLOTS; i++) { acalPointRaw[i] = NAN; acalPointN[i] = 0; acalSlotTempC[i] = NAN; acalSlotGf[i] = 1.0f; }
  TARGET_TEMP_C = AUTOCAL_TARGET_C[0];
}

// ยกเลิกโหมดคาลิเบตอัตโนมัติกลางคัน คืนอุณหภูมิเป้าหมายเป็นค่าปกติ (ผลรอบที่เสร็จแล้วยังเก็บไว้ให้ดูได้)
// v-cal-cancel: ตามคำขอผู้ใช้ ("พอกดยกเลิกการตรวจในเว็บ หน้าจอในเครื่องจะเด้งออกมาอยู่หน้าหลัก") — ก่อนหน้านี้
// ฟังก์ชันนี้แค่ตั้ง autoCalPhase = ACAL_IDLE เฉย ๆ แต่ `state` ของเครื่องยังเป็น ST_MENU_MAIN/ST_MENU_AW เดิมอยู่แล้ว
// ตลอดช่วงคาลิเบต (จงใจ ไม่สลับ state เพื่อใช้กราฟหน้าตรวจวัดปกติร่วมกัน — ดู updateAutoCalBoardGraph()) ทำให้
// เงื่อนไข `state != prevDrawnState` ใน loop() ไม่มีวันเป็นจริงอีกเลย เพราะทั้งคู่ค้างเท่ากันมาตั้งแต่ก่อนเริ่มคาลิเบต
// จอเครื่องเลยค้างเป็นกราฟคาลิเบตรอบสุดท้ายไปเรื่อย ๆ จนกว่าจะมีคนกดปุ่มที่ตัวเครื่องให้ state เปลี่ยนจริง ๆ
// แก้โดยบังคับ prevDrawnState = -1 ตรงนี้ (เหมือนที่จุดอื่น ๆ ในไฟล์ทำก่อนเปลี่ยนหน้าจอ) ให้ loop() มองว่า state
// "เปลี่ยน" แล้วในรอบถัดไปทันที -> วาดเมนูหลัก/เมนู AW ทับกราฟเดิมด้วย tft.fillScreen() ในตัว drawMenuScreen()
void cancelAutoCalMode() {
  autoCalPhase = ACAL_IDLE;
  TARGET_TEMP_C = TARGET_TEMP_C_DEFAULT;
  prevDrawnState = (AppState)-1;
}

// จบรอบปัจจุบัน: เฉลี่ยค่าในหน้าต่างล็อก 60 ตัวอย่างล่าสุด -> บันทึกผล -> ตั้ง/ตรวจ/แก้จุดชั่วคราว -> ไปรอบถัดไป
void finishAutoCalRound(bool timedOut) {
  int r = autoCalRound, slot = autoCalSlotOf(r);
  bool isVerify = autoCalIsVerifyRound(r);
  int n = (autoCalGraphCount < AUTOCAL_WIN_SAMPLES) ? autoCalGraphCount : AUTOCAL_WIN_SAMPLES;
  if (n < 1) n = 1;
  float raw, tempAvg, awMapped;
  if (acalPwmSteadyWin) {
    // v21: PWM เทลเทียร์นิ่งตลอดหน้าต่างนี้ -> ค่าที่ขึ้น-ลงเป็นแค่ ripple ของลูปควบคุม ใช้ค่าเฉลี่ยจุดสูงสุด-ต่ำสุดของ
    // การแกว่งแทนค่าเฉลี่ยเลขคณิตธรรมดา (เหตุผลเดียวกับ updateStabilityWindow() ของโหมดวัดปกติ) — ทำกับค่าดิบ/อุณหภูมิ/
    // aw ที่จับคู่กันทุกจังหวะเวลาเดียวกันเหมือนกันหมด กันจุดคาลิเบรตที่ได้เอียงจากจุดกึ่งกลางจริงของการแกว่ง
    float rmn = autoCalRawBuf[autoCalGraphCount - n], rmx = rmn;
    float tmn = autoCalTempBuf[autoCalGraphCount - n], tmx = tmn;
    float amn = autoCalGraphBuf[autoCalGraphCount - n], amx = amn;
    for (int i = autoCalGraphCount - n; i < autoCalGraphCount; i++) {
      if (autoCalRawBuf[i] < rmn) rmn = autoCalRawBuf[i]; if (autoCalRawBuf[i] > rmx) rmx = autoCalRawBuf[i];
      if (autoCalTempBuf[i] < tmn) tmn = autoCalTempBuf[i]; if (autoCalTempBuf[i] > tmx) tmx = autoCalTempBuf[i];
      if (autoCalGraphBuf[i] < amn) amn = autoCalGraphBuf[i]; if (autoCalGraphBuf[i] > amx) amx = autoCalGraphBuf[i];
    }
    raw = (rmn + rmx) / 2.0f;
    tempAvg = (tmn + tmx) / 2.0f;
    awMapped = (amn + amx) / 2.0f;
  } else {
    double sr = 0, st = 0, sa = 0;
    for (int i = autoCalGraphCount - n; i < autoCalGraphCount; i++) {
      sr += autoCalRawBuf[i]; st += autoCalTempBuf[i]; sa += autoCalGraphBuf[i];
    }
    raw = (float)(sr / n); tempAvg = (float)(st / n); awMapped = (float)(sa / n);
  }

  float gfNowRound = gradientFactor();   // v27: ตัวคูณ Magnus ณ จังหวะล็อก (ห้องนิ่งแล้ว) เก็บไว้คู่กับจุด raw ของช่องนี้
  if (isnan(gfNowRound) || gfNowRound < 0.5f || gfNowRound > 1.5f) gfNowRound = 1.0f;
  AutoCalRoundResult& res = autoCalResults[r];
  res.targetC = TARGET_TEMP_C;
  res.sampleCount = n;
  res.avgRawFrac = raw;
  res.avgAw = awMapped;
  res.avgTempC = tempAvg;
  res.isVerify = isVerify;
  res.timedOut = timedOut;
  res.passed = true;
  res.fixed = false;
  res.errAw = isnan(autoCalRefAw) ? NAN : (awMapped - autoCalRefAw);
  res.durSec = (millis() - autoCalPhaseStartMs) / 1000UL;

  if (isVerify) {
    // รอบทดสอบ: จุดชั่วคราวพาให้ aw ตรงเป้าไหม? ผ่าน = เฉลี่ย raw นี้เข้าจุดเพิ่ม / ไม่ผ่าน = แก้ใหม่ (ปรับจุดให้ตรงค่าล่าสุด)
    bool pass = !isnan(res.errAw) && fabsf(res.errAw) <= (AUTOCAL_VERIFY_TOL_AW + (acalPwmSteadyWin ? AUTOCAL_VERIFY_PWM_EXTRA_AW : 0.0f)) && !timedOut;   // v19
    res.passed = pass;
    if (pass && acalPointN[slot] > 0) {
      acalSlotTempC[slot] = (acalSlotTempC[slot] * acalPointN[slot] + tempAvg) / (acalPointN[slot] + 1);   // v27
      acalSlotGf[slot] = (acalSlotGf[slot] * acalPointN[slot] + gfNowRound) / (acalPointN[slot] + 1);
      acalPointRaw[slot] = (acalPointRaw[slot] * acalPointN[slot] + raw) / (acalPointN[slot] + 1);
      acalPointN[slot]++;
    } else {
      acalPointRaw[slot] = raw;
      acalSlotTempC[slot] = tempAvg; acalSlotGf[slot] = gfNowRound;   // v27
      acalPointN[slot] = 1;
      res.fixed = !pass;
    }
  } else {
    if (slot == 0 && acalPointN[0] > 0) {
      acalSlotTempC[0] = (acalSlotTempC[0] * acalPointN[0] + tempAvg) / (acalPointN[0] + 1);   // v27
      acalSlotGf[0] = (acalSlotGf[0] * acalPointN[0] + gfNowRound) / (acalPointN[0] + 1);
      acalPointRaw[0] = (acalPointRaw[0] * acalPointN[0] + raw) / (acalPointN[0] + 1);   // รอบวัด 25°C รอบหลัง ๆ: เฉลี่ยเข้าจุดเดิม
      acalPointN[0]++;
    } else {
      acalPointRaw[slot] = raw;   // รอบวัดแรกของช่องนี้: ตั้งจุดใหม่
      acalSlotTempC[slot] = tempAvg; acalSlotGf[slot] = gfNowRound;   // v27
      acalPointN[slot] = 1;
    }
  }
  res.pointRaw = acalPointRaw[slot];

  autoCalRound++;
  if (autoCalRound >= AUTOCAL_ROUNDS) {
    autoCalPhase = ACAL_DONE;
    TARGET_TEMP_C = TARGET_TEMP_C_DEFAULT;
    // v-cal-cancel: ครบ 10 รอบตามปกติก็ให้จอเครื่องเด้งกลับหน้าหลักทันทีเหมือนกดยกเลิก (เดิมค้างเป็นกราฟรอบสุดท้าย
    // ไปเรื่อย ๆ เช่นกัน) ผลสรุปทุกรอบยังดูได้ครบจากตารางบนเว็บอยู่แล้ว ไม่ต้องอาศัยจอเครื่องค้างไว้ให้ดู
    prevDrawnState = (AppState)-1;
  } else {
    TARGET_TEMP_C = AUTOCAL_TARGET_C[autoCalRound];
    autoCalPhase = ACAL_COOLING;
    autoCalSettleSinceMs = 0;
  }
}

// เรียกทุกรอบ loop() (ก่อน updatePeltierControl) — ถ้าไม่ได้อยู่ในโหมดนี้จะคืนทันที แทบไม่กินเวลา
void updateAutoCalMode() {
  if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) return;
  unsigned long now = millis();

  if (autoCalPhase == ACAL_COOLING) {
    bool inRange = !isnan(currentTempC) && fabs(currentTempC - TARGET_TEMP_C) <= AUTOCAL_TEMP_TOL_C;
    bool sameTarget = (autoCalRound > 0) && (AUTOCAL_TARGET_C[autoCalRound] == AUTOCAL_TARGET_C[autoCalRound - 1]);
    unsigned long need = sameTarget ? AUTOCAL_SETTLE_SAME_MS : AUTOCAL_SETTLE_MS;
    if (inRange) {
      if (autoCalSettleSinceMs == 0) autoCalSettleSinceMs = now;
      if (now - autoCalSettleSinceMs >= need) {
        autoCalPhase = ACAL_MEASURING;             // อุณหภูมินิ่งที่เป้าหมายแล้ว -> เริ่มวัดรอบนี้ (จบเมื่อค่านิ่ง)
        autoCalPhaseStartMs = now;
        autoCalSampleCount = 0;
        autoCalLastSampleMs = 0;
        autoCalGraphCount = 0;   // ล้างกราฟสดทุกครั้งที่ขึ้นรอบใหม่ ให้เห็นเฉพาะรอบปัจจุบันชัดเจน
        numPoints = 0;           // กราฟบนจอบอร์ด (ใช้ฟังก์ชันร่วมกับหน้าวัดปกติ) ก็เริ่มใหม่ทุกรอบ
        resetGraphFilter();
        acalBoardFrameDrawn = false;
        acalPrevHoldPhase = 0;   // ล้างฮิสเทอรีซิสสีเขียวของรอบก่อน
      }
    } else {
      autoCalSettleSinceMs = 0;                    // หลุดช่วงยอมรับได้ -> นับใหม่
    }
    return;
  }

  // ACAL_MEASURING: เก็บตัวอย่างทุก ~1 วิ ใช้ cache เดียวกับ handleData() กันอ่านเซนเซอร์ซ้ำซ้อน
  if (now - autoCalLastSampleMs >= 1000UL) {
    autoCalLastSampleMs = now;
    float rh;
    if (!isnan(lastShtRhPct) && now - lastShtReadMs < 900UL) {
      rh = lastShtRhPct;
    } else {
      rh = readShtHumidityRetry();
      if (!isnan(rh) && rh >= 0.0f && rh <= 100.0f) { lastShtRhPct = rh; lastShtReadMs = now; }
      else rh = lastGoodRhWeb;
    }
    if (rh >= 0.0f && rh <= 100.0f) lastGoodRhWeb = rh;
    float rawFrac = rh / 100.0f;
    float tNow = isnan(currentTempC) ? TARGET_TEMP_C : currentTempC;
    float awNow = acalMapAw(rawFrac, currentTempC);
    autoCalSampleCount++;

    // จุดกราฟสด + ค่าดิบ + อุณหภูมิ (แบบเลื่อนบัฟเฟอร์เมื่อเต็ม) — ทั้งสามอาเรย์เลื่อนพร้อมกัน
    if (autoCalGraphCount >= AUTOCAL_GRAPH_CAP) {
      for (int i = 1; i < AUTOCAL_GRAPH_CAP; i++) {
        autoCalGraphBuf[i - 1] = autoCalGraphBuf[i];
        autoCalRawBuf[i - 1] = autoCalRawBuf[i];
        autoCalTempBuf[i - 1] = autoCalTempBuf[i];
        autoCalPwmBuf[i - 1] = autoCalPwmBuf[i];
      }
      autoCalGraphCount = AUTOCAL_GRAPH_CAP - 1;
    }
    autoCalGraphBuf[autoCalGraphCount] = awNow;
    autoCalRawBuf[autoCalGraphCount] = rawFrac;
    autoCalTempBuf[autoCalGraphCount] = tNow;
    autoCalPwmBuf[autoCalGraphCount] = peltierAppliedF * 100.0f / 255.0f;
    autoCalGraphCount++;
    // ป้อนจุดเดียวกันเข้ากราฟของ "หน้าวัดปกติ" (values[]/tempValues[]) เพื่อให้จอบอร์ดวาดด้วยฟังก์ชันชุดเดียวกัน
    pushValue(awNow, currentTempC);

    // ตัดสินจบรอบ: นิ่ง (ไฟเขียวจากตัวตรวจเดียวกับโหมดวัดปกติ) + อุณหภูมิในหน้าต่างล็อกนิ่งพอ  หรือ  เกินเวลาสูงสุด
    bool timeUp = (now - autoCalPhaseStartMs) >= AUTOCAL_MAX_ROUND_MS;
    if (autoCalGraphCount >= AUTOCAL_WIN_SAMPLES) {
      float tmn = 1e9f, tmx = -1e9f;
      for (int i = autoCalGraphCount - AUTOCAL_WIN_SAMPLES; i < autoCalGraphCount; i++) {
        if (autoCalTempBuf[i] < tmn) tmn = autoCalTempBuf[i];
        if (autoCalTempBuf[i] > tmx) tmx = autoCalTempBuf[i];
      }
      // v18: PWM นิ่งตลอดหน้าต่าง -> ผ่อนช่วงกว้างอุณหภูมิที่ยอมรับ (acalPwmSteadyWin ถูกตั้งใน computeAutoCalHoldPhase())
      int holdNow = computeAutoCalHoldPhase();
      float tempTolNow = acalPwmSteadyWin ? (AUTOCAL_TEMP_WIN_TOL_C * STAB_PWM_TEMP_TOL_MULT) : AUTOCAL_TEMP_WIN_TOL_C;
      bool stable = (holdNow == 2) && ((tmx - tmn) <= tempTolNow);
      if (stable) { finishAutoCalRound(false); return; }
      if (timeUp) { finishAutoCalRound(true); return; }
    } else if (timeUp) {
      finishAutoCalRound(true);
      return;
    }
  }
}

// v-acal-graph -> v-pro -> v-shared-graph: วาดกราฟ aw สดของรอบคาลิเบตอัตโนมัติปัจจุบัน — เรียกจาก loop()
// เฉพาะตอนอยู่หน้าเมนู (เครื่องว่าง) ระหว่าง ACAL_COOLING/ACAL_MEASURING เท่านั้น ไม่รบกวนกราฟวัดค่าปกติ
// v-shared-graph: ตามคำขอผู้ใช้ ("กดคาลิเบตจากเว็บแล้วไม่ต้องเป็นหน้าจอแยกจากตรวจวัดปกติ ให้ใช้กราฟพล็อตกราฟของ
// การวัดปกติไปเลยเพื่อความง่ายและข้อมูลครบ") เปลี่ยนจากการวาดกล่องกราฟเองทั้งหมด (สเกล/เส้น/แกนแยกชุด) มาเรียกใช้
// ฟังก์ชันชุดเดียวกับที่ runMeasureAWTick() ใช้วาดหน้าตรวจวัดปกติตรงๆ (drawGraphFrame()/drawGraph()/drawCurrentValue()/
// drawCurrentTemp()) — ได้เส้นอุณหภูมิเส้นประ/แกน 0-1.0/legend "Aw · Temp" มาเหมือนกันทุกอย่างโดยไม่ต้องเขียนซ้ำ
// (ข้อมูลครบกว่าของเดิมที่ไม่เคยมีเส้นอุณหภูมิเลย) ยังคง `state` เดิม (ST_MENU_MAIN/ST_MENU_AW) ไว้ตามเดิมโดยตั้งใจ
// ไม่สลับไป ST_MEASURE_AW จริง เพื่อไม่ให้กระทบตรรกะฮีตเตอร์/PID/save-prompt/เกณฑ์นิ่งของการวัดจริง (แยกกันโดยเจตนา)
unsigned long acalBoardGraphLastMs = 0;
const unsigned long ACAL_BOARD_GRAPH_MS = 1000UL; // รีเฟรชทุก 1 วิ พอสำหรับกราฟที่ปรับทีละช้า (รอบละ 25 นาที)
void updateAutoCalBoardGraph() {
  unsigned long now = millis();
  if (now - acalBoardGraphLastMs < ACAL_BOARD_GRAPH_MS) return;
  acalBoardGraphLastMs = now;

  if (autoCalPhase == ACAL_COOLING || numPoints < 2) {
    // ยังไม่มีข้อมูลกราฟของรอบนี้พอ (กำลังรอเข้าอุณหภูมิเป้าหมาย) — เคลียร์พื้นที่กราฟแล้วโชว์ข้อความรอแทน
    tft.fillRect(0, 24, screenW, screenH - 24, COL_BG);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextDatum(TL_DATUM);
    char hdr[40];
    int roundHuman = autoCalRound + 1;
    snprintf(hdr, sizeof(hdr), "WEB CAL %d/%d %s %.0fC", roundHuman, AUTOCAL_ROUNDS, autoCalIsVerifyRound(autoCalRound) ? "TEST" : "MEAS", TARGET_TEMP_C);
    tft.drawString(hdr, 4, 4, 2);
    tft.setTextColor(COL_WARN, COL_BG);
    tft.drawString("Stabilizing temperature...", 14, screenH / 2 - 8, 2);
    char cur[24];
    snprintf(cur, sizeof(cur), "now %.1fC", isnan(currentTempC) ? 0.0f : currentTempC);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString(cur, 14, screenH / 2 + 12, 2);
    tft.setTextColor(COL_TEXT, COL_BG);
    acalBoardFrameDrawn = false;  // ให้วาดกรอบใหม่ตอนเข้าสู่ ACAL_MEASURING ของรอบนี้
    return;
  }

  if (!acalBoardFrameDrawn) {
    // วาดหัวข้อ+กรอบกราฟครั้งเดียวตอนเริ่มมีข้อมูลของรอบนี้ (เหมือน drawStaticUI() ของหน้าตรวจวัดปกติ แต่ใช้
    // หัวข้อของตัวเองบอกรอบ/อุณหภูมิเป้าหมายแทนชื่อ "Water Activity (aw)" เพื่อไม่ให้สับสนว่าเป็นการวัดจริง)
    tft.fillRect(0, 0, screenW, screenH, COL_BG);
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextDatum(TL_DATUM);
    char hdr[40];
    int roundHuman = autoCalRound + 1;
    snprintf(hdr, sizeof(hdr), "WEB CAL %d/%d %s %.0fC", roundHuman, AUTOCAL_ROUNDS, autoCalIsVerifyRound(autoCalRound) ? "TEST" : "MEAS", TARGET_TEMP_C);
    tft.drawString(hdr, 4, 4, 2);
    drawGraphFrame();   // ฟังก์ชันเดียวกับหน้าวัดปกติ: กรอบ+เส้นกริด+ป้าย 1.0/0.5/0.0+legend Aw/Temp
    acalBoardFrameDrawn = true;
  }

  float awNow = values[numPoints - 1];
  drawGraph(0);                 // ฟังก์ชันเดียวกับหน้าวัดปกติ — วาดทั้งเส้น aw และเส้นประอุณหภูมิจาก values[]/tempValues[]
  drawCurrentValue(awNow, 0);   // ตัวเลขค่าล่าสุดมุมขวาบน แบบเดียวกับหน้าวัดปกติ
  drawCurrentTemp(currentTempC);

  // เส้นประสีเหลือง = ค่าอ้างอิงเป้าหมาย (ของเดิมไม่มีในหน้าวัดปกติ จึงยังคงวาดเพิ่มเองตรงนี้ — ต้องวาดหลัง
  // drawGraph() เสมอเพราะ drawGraph() ล้างพื้นที่กราฟใหม่ทุกรอบ เหมือนกับที่ drawPromptPopupTFT() ทำ)
  if (!isnan(autoCalRefAw)) {
    int yRef = valueToY(autoCalRefAw);   // ใช้ตัวแปลงพิกัดตัวเดียวกับกราฟหลัก สเกล 0-1 ตรงกันเป๊ะ
    for (int x = graphX; x < graphX + graphW; x += 5) tft.drawPixel(x, yRef, TFT_YELLOW);
  }

  // บรรทัดค่าอ้างอิงเล็กๆ มุมล่างซ้ายของกรอบกราฟ (ตัวเลขหลักอยู่มุมขวาบนแล้วจาก drawCurrentValue())
  char foot[24];
  snprintf(foot, sizeof(foot), "ref:%s", isnan(autoCalRefAw) ? "--" : String(autoCalRefAw, 4).c_str());
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.fillRect(graphX, graphY + graphH + 2, 90, 12, COL_BG);
  tft.drawString(foot, graphX, graphY + graphH + 2, 1);
}

// v-pro: บรรทัดสถานะบน LCD 16x2 ระหว่างคาลิเบตอัตโนมัติจากเว็บ — เดิมไม่มีการอัปเดต LCD ช่วงนี้เลย (จอเครื่อง
// ทั้งจอถูกปล่อยว่างที่เมนู) ตอนนี้ให้ LCD ก็ "อิงตามเว็บ" เหมือนกับจอ TFT ด้านบน เห็นรอบ/อุณหภูมิ/ค่าล่าสุดได้ทันที
unsigned long acalLcdLastMs = 0;
const unsigned long ACAL_LCD_MS = 1000UL;
void updateAutoCalLCD() {
  unsigned long now = millis();
  if (now - acalLcdLastMs < ACAL_LCD_MS) return;
  acalLcdLastMs = now;

  int roundHuman = autoCalRound + 1;
  String l1, l2;
  if (autoCalPhase == ACAL_COOLING) {
    l1 = "CAL " + String(roundHuman) + "/" + String(AUTOCAL_ROUNDS) + (autoCalIsVerifyRound(autoCalRound) ? " TEST" : " MEAS");
    l2 = "Cool " + String(isnan(currentTempC) ? 0.0f : currentTempC, 1) + "C->" + String(TARGET_TEMP_C, 0) + "C";
  } else {
    unsigned long elapsedSec = (millis() - autoCalPhaseStartMs) / 1000UL;
    l1 = "CAL " + String(roundHuman) + "/" + String(AUTOCAL_ROUNDS) + (autoCalIsVerifyRound(autoCalRound) ? " T " : " M ") + String(elapsedSec / 60UL) + "m";
    float lastAw = autoCalGraphCount ? autoCalGraphBuf[autoCalGraphCount - 1] : NAN;
    l2 = "aw:" + (isnan(lastAw) ? String("--.----") : String(lastAw, 4));
  }
  while (l1.length() < 16) l1 += ' ';
  while (l2.length() < 16) l2 += ' ';
  lcd.setCursor(0, 0);
  lcd.print(l1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(l2.substring(0, 16));
}

bool warmupDone = false;
unsigned long belowTargetSinceMs = 0;

// ค่าฮิสเทอรีซิส (°C) — ไม่ได้ใช้แล้วตั้งแต่เปลี่ยนกลับไปใช้ PID (v8) เก็บไว้เผื่ออยากสลับกลับไปโหมด Bang-Bang ภายหลัง
const float PELTIER_HYSTERESIS_C = 0.3;

// ตรวจจับกรณี "เทลเทียร์สู้ไม่ไหว" — เปิดเต็มกำลังต่อเนื่องนานเกินไปแต่ยังไล่อุณหภูมิลงไม่ถึงเป้าหมายเลย
// (เช่น ห้องร้อนจัดเกินกำลังทำความเย็นของเทลเทียร์ตัวนี้) เพื่อเตือนผู้ใช้ตรง ๆ แทนที่จะปล่อยให้วัดค่า
// ต่อไปเงียบ ๆ ทั้งที่อุณหภูมิจริงอาจหลุดช่วงที่คาลิเบรตไว้มาก (แม้จะ clamp ใน applyCal() แล้วก็ตาม
// การ clamp ช่วยกัน error ยิ่งแย่ลง แต่ก็ยังไม่แม่นยำเท่าตอนอุณหภูมิอยู่ในช่วงจริง ผู้ใช้ควรรู้ไว้)
unsigned long peltierStuckSinceMs = 0;
const unsigned long PELTIER_STUCK_TIMEOUT_MS = 300000;  // 5 นาทีเปิดเต็มกำลังต่อเนื่องแล้วยังไม่ถึงเป้า -> ถือว่า "สู้ไม่ไหว"
bool peltierStuckHot = false;

float pid_LastTempC = NAN;  // v11: เก็บอุณหภูมิครั้งก่อน ใช้คำนวณเทอม derivative-on-measurement ของ PID มืออาชีพ

// ส่งค่า PWM ไปที่ขาเทลเทียร์: ลดกำลังได้ทันที แต่เพิ่มกำลังได้ไม่เกิน peltierSlewPerS ต่อวินาที
void peltierWrite(int target) {
  unsigned long now = millis();
  float dt = (peltierLastWriteMs == 0) ? 0.0f : (now - peltierLastWriteMs) / 1000.0f;
  if (dt > 0.5f) dt = 0.5f;   // กันกระโดดถ้าไม่ได้เรียกนาน
  peltierLastWriteMs = now;
  if ((float)target <= peltierAppliedF) {
    peltierAppliedF = (float)target;
  } else {
    peltierAppliedF += peltierSlewPerS * dt;
    if (peltierAppliedF > (float)target) peltierAppliedF = (float)target;
  }
  ledcWrite(PELTIER_PWM_PIN, (int)(peltierAppliedF + 0.5f));
}

void peltierOff() {
  peltierOutputPWM = 0;
  peltierOn = false;
  peltierAppliedF = 0;
  ledcWrite(PELTIER_PWM_PIN, 0); // ตัดกำลังไฟผ่าน PWM=0 (ละเอียดกว่ารีเลย์ ไม่มีการสวิตช์กระชาก)
  pid_Integral = 0;
  pid_LastError = 0;
  pid_LastTime = 0;
  pid_LastTempC = NAN;
  peltierStuckSinceMs = 0;
  peltierStuckHot = false;
}

// v11: "เปิดเต็มกำลังทันที" (bang-bang) — ใช้เฉพาะตอน "warmup ช่วงเปิดเครื่อง" (ให้เข้าหน้าเมนูเร็วที่สุด
// เหมือนพฤติกรรมเดิม) และตอนเครื่องว่างอยู่ที่เมนู/ยังไม่ได้เริ่มวัด (ไม่กระทบผลวัดเพราะยังไม่ได้เก็บค่า)
// ตราบใดที่อุณหภูมิยังสูงกว่าเป้าหมาย จะสั่งเต็มกำลัง 255 ทันที ให้เย็นลงเร็วที่สุดเท่าที่โมดูลขับมอเตอร์จะทำได้
// แล้วตัดปิดสนิททันทีที่ถึงเป้า (เดิมคือฟังก์ชัน runPeltierPID() ก่อน v11 — เปลี่ยนชื่อมาไว้ตรงนี้เพื่อแยก
// บทบาทให้ชัดเจนจาก PID มืออาชีพด้านล่าง ซึ่งใช้เฉพาะตอนกำลังวัดค่า aw จริงเท่านั้น)
void runPeltierBoostFull() {
  if (isnan(currentTempC)) {
    peltierOff();
    return;
  }

  unsigned long now = millis();
  float dt = (pid_LastTime == 0) ? 0.2 : (now - pid_LastTime) / 1000.0;
  if (dt <= 0) dt = 0.001; // กันหารด้วยศูนย์/ค่าติดลบกรณี millis() ยังไม่ขยับ
  pid_LastTime = now;

  float error = currentTempC - TARGET_TEMP_C;

  if (error <= 0) {
    // ถึงหรือต่ำกว่าเป้าหมายแล้ว -> ปิดสนิท และล้างค่าสะสมของ PID กันเย็นเกินเป้าไปเรื่อยๆ (integral windup)
    // (ล้างค่าไว้ด้วยเพราะ pid_Integral/pid_LastTempC ใช้ร่วมกับ runPeltierPID() มืออาชีพด้านล่าง)
    peltierOutputPWM = 0;
    peltierOn = false;
    pid_Integral = 0;
    pid_LastError = error;
    pid_LastTempC = currentTempC;
    peltierStuckSinceMs = 0;
    peltierStuckHot = false;
  } else {
    pid_Integral += error * dt;
    pid_Integral = constrain(pid_Integral, -50.0f, 50.0f); // กัน integral สะสมเกินจนพุ่งไม่หยุด (anti-windup)
    pid_LastError = error;
    pid_LastTempC = currentTempC;

    peltierOutputPWM = 255; // เปิดเต็มกำลังเสมอขณะที่ยังไม่ถึงอุณหภูมิเป้าหมาย
    peltierOn = true;

    // จับเวลาว่าเปิดเกือบเต็มกำลังต่อเนื่องมานานแค่ไหนแล้วโดยยังไม่ถึงเป้า -> นานเกิน timeout แปลว่าห้องร้อนเกินกำลังเทลเทียร์
    if (peltierOutputPWM >= 250) {
      if (peltierStuckSinceMs == 0) peltierStuckSinceMs = millis();
      if (millis() - peltierStuckSinceMs >= PELTIER_STUCK_TIMEOUT_MS) peltierStuckHot = true;
    } else {
      peltierStuckSinceMs = 0;
      peltierStuckHot = false;
    }
  }

  peltierWrite(peltierOutputPWM);   // v13: ผ่าน slew limiter
}

// v11: PID "มืออาชีพ" แบบต่อเนื่อง (Proportional-Integral-Derivative) ปรับกำลังไฟ PWM ทีละน้อยอย่างนุ่มนวล
// (ไม่กระโดดเต็มกำลัง 0/255 เหมือน bang-bang) ใช้เฉพาะตอนกำลังวัดค่า aw จริงอยู่เท่านั้น (ดู updatePeltierControl())
// เพื่อลดการแกว่งขึ้นลงของอุณหภูมิระหว่างวัด ซึ่งอาจรบกวนความนิ่งของค่าความชื้นที่อ่านได้
//   - Anti-windup: จำกัด pid_Integral ไม่ให้สะสมเกินขอบเขต กันค่าพุ่งค้างตอนออกจากช่วงควบคุมได้ (เช่นห้องร้อนจัด)
//   - Derivative-on-measurement: คำนวณอนุพันธ์จาก "อุณหภูมิที่วัดได้" ไม่ใช่จาก error โดยตรง กัน derivative
//     kick (ค่ากระชากผิดปกติ) ตอนเป้าหมายเปลี่ยนกะทันหัน — ในระบบนี้ TARGET_TEMP_C คงที่อยู่แล้ว แต่ทำไว้ให้ถูกหลัก
//   - Output clamp 0-255 เสมอ ป้องกันค่า PWM หลุดขอบเขตของ ledcWrite()
// ค่าเกน pid_Kp/pid_Ki/pid_Kd ประกาศไว้ด้านบนไฟล์ที่เดียว ปรับจูนได้ตรงนั้นโดยไม่ต้องแก้ฟังก์ชันนี้
void runPeltierPID() {
  if (isnan(currentTempC)) {
    peltierOff();
    return;
  }

  unsigned long now = millis();
  float dt = (pid_LastTime == 0) ? 0.2f : (now - pid_LastTime) / 1000.0f;
  if (dt <= 0) dt = 0.001f; // กันหารด้วยศูนย์/ค่าติดลบกรณี millis() ยังไม่ขยับ
  pid_LastTime = now;

  // v29: ใช้ T̂ และ dT/dt จาก Kalman (ถ้าเปิด+สดอยู่) — เดิม dTempDt = ผลต่างระหว่างลูป ซึ่งเป็น 0 เกือบตลอดแล้วกระโดดทีละ 1 วิ (DS18B20 อัปเดต 1 Hz)
  float tCtl = currentTempC, dTempDt;
  bool kfOk = advPidInputs(tCtl, dTempDt);
  float error = tCtl - TARGET_TEMP_C;

  if (isnan(pid_LastTempC)) pid_LastTempC = currentTempC;
  if (!kfOk) dTempDt = (currentTempC - pid_LastTempC) / dt;
  pid_LastTempC = currentTempC;

  // v29 ifix (ปิดเป็นค่าเริ่มต้น): หยุดสะสมเมื่ออิ่มตัว + เพดาน integral = กำลังไฟ 100 PWM ; ปิด = พฤติกรรมเดิมทุกประการ
  float iLim = 50.0f;
  if (advCfg.ifix) {
    iLim = (pid_Ki > 1e-4f) ? 100.0f / pid_Ki : 50.0f;
    float rawOut = pid_Kp * error + pid_Ki * pid_Integral - pid_Kd * dTempDt;
    bool sat = (rawOut >= 255.0f && error > 0) || (rawOut <= 0.0f && error < 0);
    if (!sat) pid_Integral += error * dt;
  } else {
    pid_Integral += error * dt;
  }
  pid_Integral = constrain(pid_Integral, -iLim, iLim); // anti-windup

  // error เป็นบวก (ร้อนกว่าเป้า) -> ต้องการกำลังไฟเพิ่ม, dTempDt เป็นบวก (อุณหภูมิกำลังขึ้น) -> หักลบกำลังไฟลง
  // ล่วงหน้าเพื่อชะลอไม่ให้แซงเป้าไปไกล (derivative term คุมการโอเวอร์ชูต)
  float output = pid_Kp * error + pid_Ki * pid_Integral - pid_Kd * dTempDt;
  int pwm = (int)constrain(output, 0.0f, 255.0f);
  // v29 MPC ทดลอง: ถ้าเปิดและโมเดลผ่านเกณฑ์ ให้ MPC กำหนดกำลังไฟ (PID ยังคำนวณต่อเพื่อ bumpless tracking — สลับกลับ PID ได้ไม่กระชาก)
  if (advCfg.mpc && advMpcOn && !isnan(advMpcU)) {
    pwm = (int)(advClamp(advMpcU, 0.0f, 1.0f) * 255.0f + 0.5f);
    if (pid_Ki > 1e-3f) pid_Integral = constrain((pwm - pid_Kp * error + pid_Kd * dTempDt) / pid_Ki, -iLim, iLim);
  }

  peltierOutputPWM = pwm;
  peltierOn = (pwm > 0);
  pid_LastError = error;

  // ใช้เกณฑ์เดียวกับโหมด boost: ถ้ากำลังไฟใกล้เต็ม (>=250) ต่อเนื่องนานเกิน timeout แปลว่าห้องร้อนเกินกำลังเทลเทียร์
  if (pwm >= 250) {
    if (peltierStuckSinceMs == 0) peltierStuckSinceMs = millis();
    if (millis() - peltierStuckSinceMs >= PELTIER_STUCK_TIMEOUT_MS) peltierStuckHot = true;
  } else {
    peltierStuckSinceMs = 0;
    peltierStuckHot = false;
  }

  peltierWrite(pwm);   // v13: ผ่าน slew limiter
}

// =====================================================================================
// v20: PID Auto-Tune อัตโนมัติ (Relay / Åström–Hägglund) + จำค่าที่จูนได้แยกตาม "สภาพแวดล้อม"
//   เทลเทียร์ของเครื่องนี้ทำความเย็นได้ทางเดียว (ดูหมายเหตุ v14) — ใช้เป็น "รีเลย์ไม่สมมาตร" แทนรีเลย์ปกติได้พอดี:
//   ร้อนกว่าเป้า -> เปิดกำลังสูง (ATUNE_PWM_HIGH), เย็นกว่าเป้า -> ลดกำลังต่ำ (ATUNE_PWM_LOW ไม่ปิดสนิท กันสวิตช์ถี่)
//   ความร้อนที่รั่วเข้ามาจากห้อง/ตัวอย่างเองทำหน้าที่เป็นขาตรงข้ามของรีเลย์ให้ (แทนที่จะต้องมีฮีตเตอร์จริงมาสลับ)
//   -> เกิดแกว่งเป็นวงรอบ (limit cycle) รอบเป้าหมาย วัดคาบ Pu + แอมพลิจูด a ได้ แล้วคำนวณ Ku = 4d/(pi*a)
//      จากนั้นใช้สูตร Ziegler–Nichols เดียวกับที่แผงจูนบนเว็บแนะนำผู้ใช้ทำเองไว้อยู่แล้ว
//      (Kp=0.6Ku, Ki=1.2Ku/Pu, Kd=0.075KuPu) คำนวณเกนให้อัตโนมัติ ไม่ต้องหมุนเองทีละค่า
//   เก็บผลที่ได้ (Kp/Ki/Kd) ลง NVS แยกเป็น "โปรไฟล์" ต่ออุณหภูมิห้องตอนบูต (ambientTempCatBoot ปัดเป็นองศาเต็ม)
//   เพราะห้องต่างกัน (ห้องแอร์เย็น/ห้องร้อน/ฤดูฝน-ฤดูร้อน) มีมวลความร้อน/อัตรารั่วความร้อนต่างกัน เกนที่นิ่งที่สุด
//   ของห้องหนึ่งมักไม่ใช่เกนที่นิ่งที่สุดของอีกห้อง — บูตครั้งถัดไปถ้าห้องใกล้เคียงโปรไฟล์เดิม (ปัดเป็น °C เดียวกัน)
//   จะโหลดเกนที่เคยจูนไว้ของห้องนั้นกลับมาใช้อัตโนมัติทันที ไม่ต้องจูนซ้ำทุกครั้งที่ย้ายเครื่อง/เปิดเครื่องใหม่
//   ควบคุมจากเว็บเท่านั้น (แผงแอดมิน "จูน PID เทลเทียร์"): GET /pidautotune?start=1 เริ่ม, ?cancel=1 ยกเลิก,
//   GET /pidautotune/status ดูสถานะสด — ยังปรับมือทับ (/pidset) ได้เสมอถ้าอยากจูนเองต่อจากผลอัตโนมัติ
// =====================================================================================
enum PidAutoTuneState { ATUNE_IDLE, ATUNE_RUNNING, ATUNE_DONE, ATUNE_FAILED };
PidAutoTuneState pidATuneState = ATUNE_IDLE;

const float ATUNE_PWM_HIGH = 230.0f;    // กำลังไฟ "สูง" ตอนแกว่งจูน (ไม่เอาเต็ม 255 กันโหลดกระชากเกินจำเป็น)
const float ATUNE_PWM_LOW  = 20.0f;     // กำลังไฟ "ต่ำ" ตอนแกว่งจูน (ไม่ปิดสนิท 0 กันสวิตช์ถี่จากสัญญาณรบกวนเล็ก ๆ รอบ error=0)
const float ATUNE_HYSTERESIS_C = 0.15f; // กันสวิตช์รัวเมื่ออุณหภูมิแกว่งเบา ๆ รอบเป้าหมาย (สวิตช์เมื่อห่างเป้าเกินนี้เท่านั้น)
const int   ATUNE_HALFCYCLES_SKIP = 2;  // ข้ามครึ่งรอบแรก ๆ ที่ยังไม่เข้า limit cycle จริง (เพิ่งเริ่ม/ยังลู่เข้า)
const int   ATUNE_HALFCYCLES_USE  = 6;  // ใช้ครึ่งรอบถัดจากนั้นกี่ครึ่งรอบมาเฉลี่ยหาคาบ/แอมพลิจูด (~3 รอบเต็ม)
const unsigned long ATUNE_TIMEOUT_MS = 25UL * 60UL * 1000UL;   // เพดานเวลารวมของการจูนอัตโนมัติทั้งหมด
const float ATUNE_MAX_TEMP_EXCURSION_C = 4.0f;  // แกว่งห่างเป้าหมายเกินนี้ = ผิดปกติ/อันตราย ยกเลิกทันทีเพื่อความปลอดภัย

unsigned long atuneStartMs = 0;
unsigned long atuneLastSwitchMs = 0;
bool atuneRelayHigh = false;                 // true = กำลังสั่งกำลังไฟสูงอยู่ (error > 0, กำลังไล่อุณหภูมิลง)
float atuneWindowMin = 1000, atuneWindowMax = -1000; // min/max ของครึ่งรอบปัจจุบัน (ใช้หาแอมพลิจูด)
int atuneHalfCycleCount = 0;                 // นับจำนวนครึ่งรอบ (สวิตช์ทิศ) ที่ผ่านมาแล้วตั้งแต่เริ่ม
unsigned long atunePeriodSumMs = 0;          // สะสมคาบเต็มรอบ (กลับมาเป็น High ซ้ำ) ของครึ่งรอบที่นำมาใช้เฉลี่ย
int atunePeriodSumCount = 0;
float atuneAmpSum = 0;                       // สะสมแอมพลิจูด (max-min)/2 ของครึ่งรอบที่นำมาใช้เฉลี่ย
int atuneAmpSumCount = 0;
unsigned long atuneLastFullPeriodStartMs = 0; // เวลาที่เริ่มนับคาบเต็มรอบล่าสุด
float atuneKu = 0, atunePu = 0;              // ผลลัพธ์สุดท้าย (0 = ยังไม่มี/ล้มเหลว)
float atuneEnvC = NAN;                       // อุณหภูมิห้องตอนเริ่มจูนครั้งนี้ (= โปรไฟล์ที่จะบันทึกผลลง)
float atuneSavedKp = 0, atuneSavedKi = 0, atuneSavedKd = 0; // เกนเดิมก่อนเริ่มจูน (กู้คืนได้ถ้ายกเลิก/ล้มเหลว)
String atuneFailReason = "";

// สร้างคีย์ NVS ของโปรไฟล์ตามอุณหภูมิห้อง (ปัดเป็นองศาเต็ม 0-45°C) — ใช้ชุดเดียวกันทั้งโหลด/บันทึก
void pidProfileKeys(float envC, char* kKp, char* kKi, char* kKd, size_t n) {
  int bucket = (int)roundf(constrain(envC, 0.0f, 45.0f)); // ห้องเย็นสุด-ร้อนสุดที่เครื่องนี้น่าจะเจอ
  snprintf(kKp, n, "e%dkp", bucket);
  snprintf(kKi, n, "e%dki", bucket);
  snprintf(kKd, n, "e%dkd", bucket);
}

// โหลดเกน PID ที่เคยจูนไว้ของ "สภาพแวดล้อม" (อุณหภูมิห้องตอนบูต ปัดเป็นองศาเต็ม) นี้กลับมาใช้ ถ้ามี
// เรียกใน setup() หลัง captureAmbientBaseline() เสมอ — ไม่มีผลถ้าไม่เคยจูนอัตโนมัติของห้องนี้มาก่อน (ใช้ค่าเริ่มต้นเดิมต่อ)
bool loadPidProfileForEnv(float envC) {
  if (isnan(envC)) return false;
  char kKp[16], kKi[16], kKd[16];
  pidProfileKeys(envC, kKp, kKi, kKd, sizeof(kKp));
  prefs.begin("awpid", true);
  bool found = prefs.isKey(kKp);
  if (found) {
    pid_Kp = prefs.getFloat(kKp, pid_Kp);
    pid_Ki = prefs.getFloat(kKi, pid_Ki);
    pid_Kd = prefs.getFloat(kKd, pid_Kd);
  }
  prefs.end();
  if (found) Serial.printf("[PID-ATUNE] loaded saved profile for ~%.0fC: Kp=%.2f Ki=%.3f Kd=%.2f\n", envC, pid_Kp, pid_Ki, pid_Kd);
  return found;
}

void savePidProfileForEnv(float envC, float kp, float ki, float kd) {
  if (isnan(envC)) return;
  char kKp[16], kKi[16], kKd[16];
  pidProfileKeys(envC, kKp, kKi, kKd, sizeof(kKp));
  prefs.begin("awpid", false);
  prefs.putFloat(kKp, kp);
  prefs.putFloat(kKi, ki);
  prefs.putFloat(kKd, kd);
  prefs.end();
  Serial.printf("[PID-ATUNE] saved profile for ~%.0fC: Kp=%.2f Ki=%.3f Kd=%.2f\n", envC, kp, ki, kd);
}

// เริ่มจูนอัตโนมัติ — อนุญาตเฉพาะตอนเครื่องว่างอยู่ที่เมนู (ไม่รบกวนรอบวัดจริง) และ warmup ถึงเป้าหมายแล้วครั้งหนึ่ง
bool startPidAutoTune(String &err) {
  if (pidATuneState == ATUNE_RUNNING) { err = "auto-tune is already running"; return false; }
  if (!warmupDone) { err = "wait for warmup to finish first"; return false; }
  if (!(state == ST_MENU_MAIN || state == ST_MENU_AW)) { err = "device must be idle at the menu (not mid-measurement)"; return false; }
  if (isnan(currentTempC)) { err = "no temperature reading (DS18B20 fault?)"; return false; }

  atuneSavedKp = pid_Kp; atuneSavedKi = pid_Ki; atuneSavedKd = pid_Kd; // เผื่อยกเลิก/ล้มเหลว กู้คืนเกนเดิมได้เสมอ
  atuneEnvC = isnan(ambientTempCatBoot) ? currentTempC : ambientTempCatBoot;
  atuneStartMs = millis();
  atuneLastSwitchMs = atuneStartMs;
  atuneLastFullPeriodStartMs = atuneStartMs;
  atuneRelayHigh = (currentTempC >= TARGET_TEMP_C); // เริ่มจากทิศที่ทำให้เข้าใกล้เป้าหมายก่อน
  atuneWindowMin = currentTempC; atuneWindowMax = currentTempC;
  atuneHalfCycleCount = 0;
  atunePeriodSumMs = 0; atunePeriodSumCount = 0;
  atuneAmpSum = 0; atuneAmpSumCount = 0;
  atuneKu = 0; atunePu = 0;
  atuneFailReason = "";
  pid_Integral = 0; // เริ่มจูนใหม่ ล้างค่าสะสมของ PID เดิมทิ้งก่อน (เดี๋ยวกลับมาคุมต่อด้วยเกนใหม่หลังจบ)
  pidATuneState = ATUNE_RUNNING;
  Serial.printf("[PID-ATUNE] start: env~%.1fC target=%.1fC temp=%.2fC\n", atuneEnvC, TARGET_TEMP_C, currentTempC);
  return true;
}

void cancelPidAutoTune(bool restoreOldGains) {
  if (pidATuneState != ATUNE_RUNNING) return;
  if (restoreOldGains) { pid_Kp = atuneSavedKp; pid_Ki = atuneSavedKi; pid_Kd = atuneSavedKd; }
  pid_Integral = 0;
  pidATuneState = ATUNE_IDLE;
  Serial.println("[PID-ATUNE] cancelled - restored previous gains");
}

// เรียกทุกรอบจาก updatePeltierControl() แทน runPeltierPID()/runPeltierBoostFull() ตราบเท่าที่กำลังจูนอยู่ (ATUNE_RUNNING)
void runPidAutoTuneStep() {
  if (isnan(currentTempC)) { pidATuneState = ATUNE_FAILED; atuneFailReason = "temperature reading lost mid-tune"; peltierOff(); return; }

  unsigned long now = millis();

  // เพดานเวลารวม / แกว่งเกินขอบเขตปลอดภัย -> ยกเลิกเป็นล้มเหลว กู้เกนเดิมคืน กันเครื่องแกว่งค้างไม่จบไม่สิ้น
  if (now - atuneStartMs > ATUNE_TIMEOUT_MS) {
    pid_Kp = atuneSavedKp; pid_Ki = atuneSavedKi; pid_Kd = atuneSavedKd; pid_Integral = 0;
    pidATuneState = ATUNE_FAILED; atuneFailReason = "timed out - room did not settle into a steady oscillation";
    return;
  }
  if (fabsf(currentTempC - TARGET_TEMP_C) > ATUNE_MAX_TEMP_EXCURSION_C) {
    pid_Kp = atuneSavedKp; pid_Ki = atuneSavedKi; pid_Kd = atuneSavedKd; pid_Integral = 0;
    pidATuneState = ATUNE_FAILED; atuneFailReason = "temperature swung too far from target - aborted for safety";
    return;
  }

  if (currentTempC < atuneWindowMin) atuneWindowMin = currentTempC;
  if (currentTempC > atuneWindowMax) atuneWindowMax = currentTempC;

  float err = currentTempC - TARGET_TEMP_C;
  bool wantHigh = atuneRelayHigh ? (err > -ATUNE_HYSTERESIS_C) : (err > ATUNE_HYSTERESIS_C);

  if (wantHigh != atuneRelayHigh) {
    // สวิตช์ทิศ = จบครึ่งรอบหนึ่ง
    atuneHalfCycleCount++;
    if (atuneHalfCycleCount > ATUNE_HALFCYCLES_SKIP) {
      float amp = (atuneWindowMax - atuneWindowMin) / 2.0f;
      atuneAmpSum += amp; atuneAmpSumCount++;
      if (wantHigh && atuneHalfCycleCount > ATUNE_HALFCYCLES_SKIP + 1) { // กลับมาเป็น High อีกครั้ง = ครบ 1 คาบเต็มพอดี
        atunePeriodSumMs += (now - atuneLastFullPeriodStartMs);
        atunePeriodSumCount++;
      }
      if (wantHigh) atuneLastFullPeriodStartMs = now;
    }
    atuneRelayHigh = wantHigh;
    atuneWindowMin = currentTempC; atuneWindowMax = currentTempC;
    atuneLastSwitchMs = now;

    if (atuneHalfCycleCount >= ATUNE_HALFCYCLES_SKIP + ATUNE_HALFCYCLES_USE && atunePeriodSumCount > 0 && atuneAmpSumCount > 0) {
      float a = atuneAmpSum / atuneAmpSumCount;                              // แอมพลิจูดเฉลี่ยของอุณหภูมิ (°C)
      float pu = (atunePeriodSumMs / (float)atunePeriodSumCount) / 1000.0f;  // คาบเฉลี่ย (วินาที)
      float d = (ATUNE_PWM_HIGH - ATUNE_PWM_LOW) / 2.0f;                     // แอมพลิจูดรีเลย์ (หน่วย PWM 0-255)
      if (a > 0.02f && pu > 1.0f) {
        float ku = (4.0f * d) / (3.14159265f * a);  // Åström–Hägglund: Ku = 4d/(pi*a)
        atuneKu = ku; atunePu = pu;
        // Ziegler–Nichols แบบเดียวกับที่แผงจูนเว็บแนะนำผู้ใช้ทำเองไว้อยู่แล้ว (ดู pidTuneCard บนเว็บ)
        float newKp = constrain(0.6f * ku, 0.0f, 200.0f);
        float newKi = constrain(1.2f * ku / pu, 0.0f, 20.0f);
        float newKd = constrain(0.075f * ku * pu, 0.0f, 100.0f);
        pid_Kp = newKp; pid_Ki = newKi; pid_Kd = newKd; pid_Integral = 0;
        savePidProfileForEnv(atuneEnvC, newKp, newKi, newKd);
        pidATuneState = ATUNE_DONE;
        Serial.printf("[PID-ATUNE] done: Ku=%.2f Pu=%.1fs -> Kp=%.2f Ki=%.3f Kd=%.2f (env~%.0fC)\n", ku, pu, newKp, newKi, newKd, atuneEnvC);
      } else {
        pid_Kp = atuneSavedKp; pid_Ki = atuneSavedKi; pid_Kd = atuneSavedKd; pid_Integral = 0;
        pidATuneState = ATUNE_FAILED; atuneFailReason = "oscillation too small/noisy to measure reliably";
      }
      return;
    }
  }

  peltierOutputPWM = (int)(atuneRelayHigh ? ATUNE_PWM_HIGH : ATUNE_PWM_LOW);
  peltierOn = (peltierOutputPWM > 0);
  peltierWrite(peltierOutputPWM);
}

// เรียกทุกรอบ loop(): อุณหภูมิห้องวัดต่ำกว่าเป้าหมายเกิน COLD_ROOM_MARGIN_C ต่อเนื่อง 2 นาที = ห้องเย็นเกินกว่าเครื่องจะคุมให้ถึงเป้าได้
void updateColdRoomFlag() {
  if (!isnan(currentTempC) && currentTempC < TARGET_TEMP_C - COLD_ROOM_MARGIN_C) {
    if (coldRoomSinceMs == 0) coldRoomSinceMs = millis();
    if (millis() - coldRoomSinceMs >= 120000UL) coldRoomWarn = true;
  } else {
    coldRoomSinceMs = 0;
    coldRoomWarn = false;
  }
}

void updatePeltierControl() {
  advControlTick();   // v29: ระบุโมเดลเทลเทียร์ออนไลน์ + MPC (ไม่ทำอะไรถ้าไม่มีข้อมูล)
  // v13: ช่วงหลังบูตใหม่ ๆ ห้ามเทลเทียร์/พัดลมทำงาน (peltierStartAllowedMs) เพื่อไม่ให้กินกระแสซ้อนกับ Wi-Fi/จอ ตอนไฟเลี้ยงยังไม่นิ่ง
  if (millis() < peltierStartAllowedMs) {
    if (peltierAppliedF > 0) peltierOff();
    return;
  }
  // v20: กำลังจูน PID อัตโนมัติอยู่ -> ให้ลูปรีเลย์ (runPidAutoTuneStep) คุมเทลเทียร์แทนทุกอย่างด้านล่างนี้ทั้งหมด
  // จนกว่าจะจบ (สำเร็จ/ล้มเหลว/ถูกยกเลิก) — ไม่ต้องเช็ค warmupDone/isActivelyMeasuring เพราะ startPidAutoTune()
  // อนุญาตให้เริ่มเฉพาะตอน warmup เสร็จแล้วและเครื่องว่างอยู่ที่เมนูเท่านั้นอยู่แล้ว
  if (pidATuneState == ATUNE_RUNNING) { runPidAutoTuneStep(); return; }

  if (!warmupDone) {
    // ช่วง warmup ตอนเปิดเครื่อง: ใช้ bang-bang เต็มกำลังเสมอ ให้ไล่อุณหภูมิเข้าเป้าหมายเร็วที่สุด
    // แล้วเข้าหน้าเมนูหลักได้เร็วเหมือนพฤติกรรมเดิม ไม่ใช้ PID นุ่มนวลตอนนี้เพราะจะช้ากว่า
    if (isnan(currentTempC)) {
      belowTargetSinceMs = 0;
      runPeltierBoostFull();
      return;
    }
    if (currentTempC <= TARGET_TEMP_C) {
      if (belowTargetSinceMs == 0) belowTargetSinceMs = millis();
      if (millis() - belowTargetSinceMs >= WARMUP_HOLD_MS) {
        warmupDone = true;
        peltierOff();
        return;
      }
    } else {
      belowTargetSinceMs = 0;
    }
    runPeltierBoostFull();
    return;
  }

  // หลัง warmup เสร็จแล้ว: ถ้ากำลังวัดค่า aw จริงอยู่ (ปกติ/ทำนาย/เปรียบเทียบ) ใช้ PID มืออาชีพแบบต่อเนื่อง
  // เพื่อคุมอุณหภูมิให้นิ่งและนุ่มนวลที่สุด ลดการแกว่งที่จะรบกวนค่าที่กำลังวัด ส่วนตอนอยู่ที่เมนู/ว่าง (ยังไม่ได้
  // เริ่มวัด) ใช้ bang-bang เต็มกำลังเหมือนเดิม เพื่อไล่กลับเข้าเป้าหมายให้เร็วที่สุดโดยไม่กระทบผลวัดใด ๆ
  // v12: ช่วงฮีต/รอเย็นเซนเซอร์ (ST_SENSOR_COND) ใช้ PID ต่อเนื่องเช่นกัน เพื่อให้อุณหภูมิห้องวัดนิ่งอยู่แล้วตั้งแต่วินาทีแรกที่เริ่มวัดจริง
  bool isActivelyMeasuring = (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND);
  if (isActivelyMeasuring) runPeltierPID();
  else runPeltierBoostFull();
}

// ============================================================================
//  v29 ADV glue — ต่อโมดูล ADV เข้ากับเฟิร์มแวร์ (ทุกฟีเจอร์ปิด/เปิดได้จาก GET /adv/set และจำค่าใน NVS)
//   kf   = Kalman filter ของ DS18B20 -> ใช้ T̂ และ dT/dt ที่เรียบในลูป PID (แทนผลต่างดิบที่กระตุกวินาทีละครั้ง)   [ค่าเริ่มต้น เปิด]
//   ifix = PID: หยุดสะสม integral เมื่อเอาต์พุตอิ่มตัว + เพดาน integral ตามกำลังไฟ (เดิมเพดาน 50 x Ki = แค่ 5 PWM เมื่อ Ki=0.1) [ค่าเริ่มต้น ปิด]
//   mpc  = MPC ทดลอง: ใช้ผลระบุโมเดลออนไลน์ ถ้าโมเดลไม่ผ่านเกณฑ์จะถอยกลับไปใช้ PID เองอัตโนมัติ                      [ค่าเริ่มต้น ปิด]
//   EKF (aw/ความชัน/delta) และ TinyML (ค่าสมดุลรอง) เป็น "ผู้สังเกตการณ์" ไม่แตะค่าที่แสดง/บันทึก -> ดูที่ GET /adv/status
// ============================================================================
static AdvKF2 advKf;
static unsigned long advKfLastMs = 0;
static AdvEKF5 advEkf;
static unsigned long advEkfLastMs = 0;
static AdvPlantFit advId;
static AdvMpc advMpc;
static float advUAcc = 0.0f, advUAccT = 0.0f;
static unsigned long advUAccLastMs = 0;
static float advMlEqRaw = NAN, advMlEqAw = NAN, advMlDisagree = NAN;
static bool  advReady = false;

static void advInitOnce() {
  if (advReady) return;
  advKf.config(5e-5f, 5e-4f, 9.0f);
  advEkf.config();
  advId.reset();
  advMpc.config();
  advReady = true;
}
static void advLoadCfg() {
  advInitOnce();
  prefs.begin("advcfg", true);
  advCfg.kf   = prefs.getBool("kf", true);
  advCfg.ifix = prefs.getBool("ifix", false);
  advCfg.mpc  = prefs.getBool("mpc", false);
  prefs.end();
}
static void advSaveCfg() {
  prefs.begin("advcfg", false);
  prefs.putBool("kf", advCfg.kf); prefs.putBool("ifix", advCfg.ifix); prefs.putBool("mpc", advCfg.mpc);
  prefs.end();
}
// ---- Kalman ของ DS18B20 ----
void advKfUpdate(float t) {
  advInitOnce();
  unsigned long now = millis();
  float dt = advKfLastMs ? (now - advKfLastMs) / 1000.0f : 1.0f;
  advKfLastMs = now;
  advKf.update(t, dt);
}
void advKfReset() { advInitOnce(); advKf.reset(); advKfLastMs = 0; advMpcOn = false; }
static bool advKfFresh() { return advKf.init && advKfLastMs != 0 && (millis() - advKfLastMs) < 3000UL && !sensorFaultDS18B20; }
// ให้ PID: อุณหภูมิ/อนุพันธ์ที่กรองแล้ว — คืน false = ใช้วิธีเดิม
bool advPidInputs(float& tHat, float& dTdt) {
  if (!advCfg.kf || !advKfFresh()) return false;
  tHat = advKf.Tat((millis() - advKfLastMs) / 1000.0f);
  dTdt = advKf.Tdot();
  return advOk(tHat) && advOk(dTdt);
}
// ---- EKF ของ aw (ผู้สังเกตการณ์) ----
void advEkfFeed(float raw) {
  advInitOnce();
  if (!advOk(raw) || raw < 0) return;
  unsigned long now = millis();
  float dt = advEkfLastMs ? (now - advEkfLastMs) / 1000.0f : 1.0f;
  advEkfLastMs = now;
  float tc = (!isnan(shtTempC) && (now - lastShtTempMs) < 5000UL) ? shtTempC : NAN;
  float ts = (!sensorFaultDS18B20 && !isnan(currentTempC)) ? currentTempC : NAN;
  if (isnan(tc) || isnan(ts)) return;
#if AW_RAW_MODE
  float rhCal = raw;
#else
  float rhCal = calTableLookup(raw) ;
#endif
  advEkf.step(rhCal, tc, ts, dt);
}
// ---- ระบุโมเดล + MPC: เรียกทุกรอบ loop() จาก updatePeltierControl() ----
void advControlTick() {
  advInitOnce();
  unsigned long now = millis();
  if (advUAccLastMs == 0) advUAccLastMs = now;
  float dt = (now - advUAccLastMs) / 1000.0f; advUAccLastMs = now;
  if (dt > 1.0f) dt = 1.0f;
  advUAcc += (peltierAppliedF / 255.0f) * dt; advUAccT += dt;
  if (advUAccT < 4.0f) return;
  float uAvg = advUAcc / advUAccT; advUAcc = 0.0f; advUAccT = 0.0f;
  if (!advKfFresh()) { advMpcOn = false; return; }
  float That = advKf.Tat((millis() - advKfLastMs) / 1000.0f);
  advId.push(That, advClamp(uAvg, 0.0f, 1.0f));
  advMpcOn = false;
  if (!advCfg.mpc || !advId.valid() || !advId.cInit || pidATuneState == ATUNE_RUNNING || !warmupDone) return;
  const float* th = advId.theta();
  float y0 = That - TARGET_TEMP_C, ym1 = advId.yb[1] - TARGET_TEMP_C;
  float cDev = advId.cAbs - TARGET_TEMP_C * (1.0f - th[0] - th[1]);
  float u = advMpc.solve(th, advId.best, y0, ym1, advId.ub, cDev, peltierSlewPerS / 255.0f * 4.0f);
  if (!advOk(u)) return;
  advMpcU = advClamp(u, advMpcU - 0.15f, advMpcU + 0.15f);   // จำกัดการเปลี่ยนต่อ 4 วิ (ผ่านการจูนในจำลองแล้ว)
  advMpcOn = true;
}
// ---- TinyML: ค่าสมดุลรองจาก MLP (residual ของ AR) — เรียกจาก computeEquilibriumPrediction() ----
void advMlUpdate(const float* buf, int n, float arEqRaw) {
  float x[ADV_ML_NI];
  if (!advMlFeatures(buf, n, arEqRaw, x)) { advMlEqRaw = advMlEqAw = advMlDisagree = NAN; return; }
  AdvMlp m; m.attach(ADV_MLP_W1, ADV_MLP_B1, ADV_MLP_W2, ADV_MLP_B2);
  float eq = buf[n - 1] + m.infer(x);
  if (!(eq >= 0.0f && eq <= 1.0f)) { advMlEqRaw = advMlEqAw = advMlDisagree = NAN; return; }
  advMlEqRaw = eq;
  advMlEqAw = applyCal(eq, currentTempC);
  advMlDisagree = fabsf(eq - arEqRaw);
}
void advMlClear() { advMlEqRaw = advMlEqAw = advMlDisagree = NAN; }


void showWelcomeScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  const char* title1 = " SYSTEM BOOTING ";
  // v-stability: เดิม delay() ยาว ๆ หลายจุดในฟังก์ชันนี้ทำให้ server.handleClient() ไม่ถูกเรียกช่วงบูต
  // เว็บแดชบอร์ดจึงค้างชั่วคราว — แทรก server.handleClient()+watchdog reset เข้าไปในทุกลูปที่มี delay()
  for (int i = 0; title1[i] != '\0'; i++) { lcd.print(title1[i]); delay(40); server.handleClient(); esp_task_wdt_reset(); }
  lcd.setCursor(0, 1);
  lcd.print("   PLEASE WAIT   ");

  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WATER ACTIVITY METER", screenW / 2, 8, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(AW_RAW_MODE ? "Version 2.0 (PID) RAW" : "Version 2.0 (PID)", screenW / 2, 24, 1);

  // ตำแหน่ง/ขนาดมาสคอต ปรับ divisor ได้ตามความละเอียดจอจริงของบอร์ด
  int mascotSplashCX = screenW / 2;
  int mascotSplashCY = screenH / 2 - 4;
  int mascotSplashR = min(screenW, screenH) / 5;
  if (mascotSplashR < 18) mascotSplashR = 18;

  drawMascotSplashEntrance(mascotSplashCX, mascotSplashCY, mascotSplashR);

  int barX = 40; int barY = screenH - 44;
  int barW = screenW - 80; int barH = 10;
  if (barW < 20) barW = screenW - 20;
  tft.drawRect(barX - 2, barY - 2, barW + 4, barH + 4, TFT_DARKGREY);

  unsigned long lastBlinkMs = millis();
  bool eyesOpen = true;
  for (int step = 0; step <= barW; step += 3) {
    tft.fillRect(barX, barY, step, barH, tft.color565(255, 150 + (step / 3), 0));
    tft.fillRect(0, barY - 26, screenW, 16, TFT_BLACK);
    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    if (step < barW * 0.3) tft.drawString("Connecting Humidity Sensor...", screenW / 2, barY - 22, 1);
    else if (step < barW * 0.7) tft.drawString("Initializing DS18B20 Async...", screenW / 2, barY - 22, 1);
    else tft.drawString("Loading PID Controller Key...", screenW / 2, barY - 22, 1);

    // มาสคอตกระพริบตาเป็นระยะระหว่างรอโหลด ให้ดูมีชีวิตชีวา
    if (millis() - lastBlinkMs > 900) {
      eyesOpen = !eyesOpen;
      lastBlinkMs = millis();
      drawMascotFace(mascotSplashCX, mascotSplashCY, mascotSplashR, eyesOpen);
    }
    server.handleClient();
    esp_task_wdt_reset();
    delay(20);
  }
  if (!eyesOpen) drawMascotFace(mascotSplashCX, mascotSplashCY, mascotSplashR, true);

  lcd.setCursor(0, 0);
  lcd.print("  AW-METER READY ");
  lcd.setCursor(0, 1); lcd.print("  ENTER TO MENU  ");
  tft.fillRect(0, barY - 26, screenW, 16, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("SYSTEM READY!", screenW / 2, barY - 22, 1);
  setLED(false, true, false); delay(800); setLED(false, false, false);
  lcd.clear(); tft.fillScreen(TFT_BLACK); tft.setTextDatum(TL_DATUM);
}

void formatTempC(char* out, size_t outSize) {
  if (isnan(currentTempC)) snprintf(out, outSize, " --.-C");
  else if (isTempOutOfCalRange(currentTempC)) snprintf(out, outSize, "%4.1fC!", currentTempC);  // "!" เตือนว่าหลุดช่วงคาลิเบรต
  else snprintf(out, outSize, "%5.1fC", currentTempC);
}

// v11: ฟอร์แมตระยะเวลาที่ผ่านไปนับจาก startMs (mm:ss) สำหรับโชว์บนจอ LCD 16 ตัวอักษร
// ใช้แทนที่บรรทัดหมวดอาหารเดิมในทุกโหมดที่ "กำลังวัด" อยู่ (วัดปกติ/ทำนาย/เปรียบเทียบ)
// จำกัด mm ไว้ไม่เกิน 99 นาที กันข้อความล้นจอในกรณีที่วัดค้างไว้นานผิดปกติ
void formatElapsedTimeLCD(unsigned long startMs, char* out, size_t outSize) {
  unsigned long elapsedSec = (millis() - startMs) / 1000UL;
  unsigned int mm = (unsigned int)(elapsedSec / 60UL);
  unsigned int ss = (unsigned int)(elapsedSec % 60UL);
  if (mm > 99) mm = 99;
  snprintf(out, outSize, "Time %02u:%02u", mm, ss);
}

// v14: ฟอร์แมตเวลาที่ใช้วัด "แบบค่าคงที่ที่จับไว้แล้ว" (ไม่ใช่เวลานับสดแบบ formatElapsedTimeLCD ด้านบน)
// ใช้กับหน้าผลลัพธ์/บันทึกที่วัดเสร็จแล้ว — durationSec = 0 หมายถึงระเบียนเก่าที่บันทึกไว้ก่อนมีฟีเจอร์นี้
// (ยังไม่มีข้อมูลเวลาเก็บไว้) จึงโชว์ "--:--" แทนเพื่อไม่ให้เข้าใจผิดว่าใช้เวลาวัด 0 วินาทีจริง ๆ
void formatDurationShort(unsigned long durationSec, char* out, size_t outSize) {
  if (durationSec == 0) {
    snprintf(out, outSize, "Time --:--");
    return;
  }
  unsigned int mm = (unsigned int)(durationSec / 60UL);
  unsigned int ss = (unsigned int)(durationSec % 60UL);
  if (mm > 99) mm = 99;
  snprintf(out, outSize, "Time %02u:%02u", mm, ss);
}

bool bootStaticDrawn = false;
unsigned long lastBootUIUpdateMs = 0;
const unsigned long BOOT_UI_UPDATE_MS = 300;

void drawBootWarmupStatic() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("WATER ACTIVITY METER", screenW / 2, 12, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char tgt[26];
  snprintf(tgt, sizeof(tgt), "Target: <= %.1f C", TARGET_TEMP_C);
  tft.drawString(tgt, screenW / 2, screenH - 16, 1);
  tft.setTextDatum(TL_DATUM);
  bootStaticDrawn = true;
}

void drawBootWarmupDynamic() {
  char tbig[8];
  if (isnan(currentTempC)) snprintf(tbig, sizeof(tbig), "--.-C");
  else snprintf(tbig, sizeof(tbig), "%.1fC", currentTempC);
  tft.fillRect(0, screenH / 2 - 30, screenW, 40, TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(tbig, screenW / 2, screenH / 2 - 26, 4);
  tft.fillRect(0, screenH / 2 + 18, screenW, 20, TFT_BLACK);
  if (warmupDone) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("TEMP READY!", screenW / 2, screenH / 2 + 20, 2);
  } else if (peltierStuckHot) {
    // เทลเทียร์เปิดเต็มกำลังนานเกิน 5 นาทีแล้วแต่ยังไล่อุณหภูมิไม่ถึงเป้า -> ห้องร้อนเกินกำลังเครื่อง
    tft.setTextColor(COL_WARN, TFT_BLACK);
    tft.drawString("ROOM TOO HOT!", screenW / 2, screenH / 2 + 20, 2);
  } else {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tft.drawString("ADJUSTING TEMP...", screenW / 2, screenH / 2 + 20, 2);
  }
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char tbuf[8];
  formatTempC(tbuf, sizeof(tbuf));
  String l1 = "Temp:" + String(tbuf);
  while (l1.length() < 16) l1 += ' ';
  lcd.setCursor(0, 0);
  lcd.print(l1.substring(0, 16));
  String l2 = warmupDone ? "Temp Ready!" : (peltierStuckHot ? "Room too hot!  " : "Adjusting temp..");
  while (l2.length() < 16) l2 += ' ';
  lcd.setCursor(0, 1);
  lcd.print(l2.substring(0, 16));
}

unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // v7: ลดจาก 200ms -> 100ms ให้ตรวจจับ/รีเฟรชค่าเร็วขึ้น (คู่กับ oversample ที่ลดลง)
// v-cal-fix: เดิม 30000 (30 วิ) แต่จุดตัดสิน "นิ่ง" จริง (ดู updateStabilityWindow() ด้านล่าง) เดิมอิงกราฟบนจอ TFT
// ซึ่งเก็บได้แค่ ~15-22 วิล่าสุดเท่านั้น (SAVE_PROMPT_WINDOW_PTS=150 จุด x 100ms) — จากข้อมูลจริงที่ผู้ใช้เก็บมา
// (เช่น น้ำเปล่า/แคลเซียมคลอไรด์ ที่กราฟยังลากลงต่อเนื่องถึงนาทีที่ 15-40) หน้าต่าง 15-22 วิ สั้นเกินไปมาก
// ช่วงที่กราฟกำลังลดลงช้า ๆ (slow decay) จะดู "แบนนิ่ง" ในกรอบเวลาสั้น ๆ ได้ง่าย ทั้งที่ยังไม่ถึงจุดสมดุลจริง
// ทำให้เด้งป็อปอัป/ล็อกค่าเร็วเกินไป — เป็นสาเหตุหลักที่ทำให้ค่า raw ที่วัดซ้ำของตัวอย่างอ้างอิงเดียวกันกระจายมาก
// ตอนนี้ขยับเป็น 5 นาที และเปลี่ยนตัวตัดสิน "นิ่ง" ให้ใช้บัฟเฟอร์ stabBuf (ยาวเท่าหน้าต่างนี้จริง) แทนกราฟบนจอ
// (ดู updateStabilityWindow()) — ถ้าอยากรอนานขึ้น/สั้นลงอีก ปรับตัวเลขนี้ที่เดียว
// v16 (ขั้นที่ 1): หน้าต่างตัดสิน "นิ่ง" ลดจาก 5 นาที -> 60 วิ (ดูตัวตรวจนิ่งแบบบล็อกด้านล่าง engine "v16 stability")
// เดิมต้องรอค่าคงที่ครบ 5 นาที + วัดขั้นต่ำ 5 นาที แล้วยังใช้เกณฑ์ LED (0.008) หลวมกว่าเกณฑ์ล็อกจริง (0.0008) 10 เท่า
// v23: แก้บั๊ก "บัฟเฟอร์แคบเกินไปจนตรวจไม่เจอว่านิ่ง" — 60 วิ แคบกว่าคาบการแกว่งจริงของลูป PID (จากกราฟจริง ~110-120 วิ/คาบ
// เมื่อ PWM ยังสวิตช์เต็ม 0%/100% เป็นช่วง ๆ) ไปครึ่งหนึ่ง หน้าต่าง 60 วิจึงมักจับได้แค่ครึ่งขาขึ้นหรือขาลงของรอบ
// การแกว่งเดียวเท่านั้น ไม่เคยเห็น "ภาพเต็มคาบ" ที่แบนจริง (ทั้งที่ PID เข้าสมดุลแล้ว) โดยเฉพาะเห็นชัดในโหมดคาลิเบต
// อัตโนมัติที่ค้างสถานะ "ยังเคลื่อนที่" อยู่นานจนเกือบครบเพดาน 30 นาที/รอบ — ขยับเป็น 4 นาที (~2 คาบเต็ม) ให้มีโอกาส
// ครอบตำแหน่งเริ่มต้นของหน้าต่างได้ครบอย่างน้อย 1 คาบเต็มเสมอไม่ว่าจะเริ่มนับตรงจังหวะไหนของการแกว่ง
const unsigned long STAB_WINDOW_MS = 240000UL;  // v23: 4 นาที (เดิม 60 วิ)

// ---------- v12: เกณฑ์ "กราฟนิ่ง" -> เด้งป็อปอัปถามบนจอบอร์ด ----------
// วัดจากเส้น aw ที่วาดอยู่บนกราฟ TFT จริง ๆ (values[] หลังผ่านตัวกรอง EMA แล้ว) ช่วง SAVE_PROMPT_WINDOW_PTS จุดล่าสุด
// ถ้าช่วงกว้าง (max - min) <= SAVE_PROMPT_TOL ถือว่ากราฟแบนแล้ว (0.001 = 1 หลักสุดท้ายของค่า aw ที่โชว์บนจอ)
// ผู้ใช้ระบุช่วง 0.0005 - 0.001 จึงตั้งกลาง ๆ ที่ 0.0008 — อยากให้เด้งง่ายขึ้นให้เพิ่มค่า / เข้มขึ้นให้ลดค่า
// (ถ้าเซนเซอร์มี noise มากจนไม่เคยเด้ง ลองขยับไปที่ 0.001) หมายเหตุ: 1 จุด = 1 รอบอ่าน (~100 ms + เวลาอ่านเซนเซอร์)
const float SAVE_PROMPT_TOL = 0.0008f;
const int SAVE_PROMPT_WINDOW_PTS = 150;

// คืนช่วงกว้างของเส้น aw บนกราฟใน n จุดล่าสุด (คืน -1 ถ้ายังมีจุดไม่ครบ n)
float graphSteadyRange(int n) {
  if (n <= 1 || numPoints < n) return -1.0f;
  float mn = values[numPoints - 1], mx = values[numPoints - 1];
  for (int i = numPoints - n; i < numPoints; i++) {
    if (values[i] < mn) mn = values[i];
    if (values[i] > mx) mx = values[i];
  }
  return mx - mn;
}
// จำนวนจุดที่ใช้ตรวจ (ไม่เกินความกว้างกราฟบนจอ กันจอเล็กที่กราฟมีจุดไม่ถึง 150)
int savePromptWindowPts() {
  return (graphW < SAVE_PROMPT_WINDOW_PTS) ? graphW : SAVE_PROMPT_WINDOW_PTS;
}
// v12: แก้บั๊ก — เดิมตั้งไว้ 200 แต่ neededSamples = STAB_WINDOW_MS / updateInterval = 30000/100 = 300 ทำให้
// stabBufCount ไม่เคยถึง neededSamples และค่าไม่เคยถูกตัดสินว่า "นิ่ง" เลย ตอนนี้เผื่อไว้เกินจำนวนที่ต้องใช้เสมอ
// v-cal-fix: STAB_WINDOW_MS ขยับเป็น 5 นาทีแล้ว (ดูด้านบน) -> ต้องการ 300000/100 = 3000 ตัวอย่าง เผื่อไว้ 3200
// (float 2 อาเรย์ x 3200 ตัว ~= 25.6KB RAM — สบาย ๆ สำหรับ ESP32) อ้างอิงค่าคงที่จริงแทนเลข 30000/100 ตายตัว
#define STAB_BUF_CAP 2500  // v23: STAB_WINDOW_MS ใหม่ (4 นาที) ต้องการ 240000/100 = 2400 ตัวอย่าง เผื่อไว้ 2500 (ใช้เฉพาะเตือน noise) — ~20 KB RAM
static_assert(STAB_BUF_CAP >= (STAB_WINDOW_MS / updateInterval), "STAB_BUF_CAP must hold STAB_WINDOW_MS/updateInterval samples");
float stabBuf[STAB_BUF_CAP];  // เก็บค่า "ดิบ" (raw RH เศษส่วน) ไม่ใช่ค่าที่ผ่านคาลิเบรตแล้ว
int stabBufCount = 0, stabBufHead = 0;

// ---- บัฟเฟอร์นิ่งของอุณหภูมิ (v4 เพิ่มใหม่) ----
// เหตุผลสำคัญ: ถ้าเช็คแค่ RH นิ่ง เครื่องอาจ "ล็อกค่าปลอม" ตอนที่ Peltier ยังไล่อุณหภูมิ
// อยู่ช้า ๆ (เช่นเหลือ 0.01°C/วิ) เพราะ RH ก็จะนิ่งตามไปด้วยชั่วคราวโดยยังไม่ถึงจุดสมดุลไอน้ำจริง
// ต้องให้อุณหภูมิ "นิ่งพร้อมกัน" กับ RH เสมอ ถึงจะยอมล็อกค่าได้
const float STAB_TEMP_RANGE_TOL_C = 0.4;  // ยอมให้อุณหภูมิแกว่งได้ไม่เกินนี้ในหน้าต่างเวลาเดียวกับ RH
float stabTempBuf[STAB_BUF_CAP];
int stabTempBufCount = 0, stabTempBufHead = 0;

bool measureStable = false;
float measureFinalAw = 0;
// v14: จับเวลาที่ใช้วัดจริง (วินาที) ณ จังหวะที่ค่า "นิ่ง" ถูกล็อก ใช้แทนหมวดอาหารในหน้าผลลัพธ์/บันทึก
unsigned long measureFinalDurationSec = 0;
bool measureTempWasStableToo = true; // เก็บไว้โชว์ผู้ใช้เผื่ออยากรู้ว่าตอนล็อกค่า อุณหภูมินิ่งจริงหรือ (สำหรับ debug/บันทึก)

// ต้องวัดต่อเนื่องนานเกิน 5 นาที ก่อนจะยอมให้ล็อกค่าว่า "นิ่ง" ได้ แม้ค่าดิบจะนิ่งเร็วกว่านั้นก็ตาม
const unsigned long MIN_MEASURE_DURATION_MS = 5UL * 60UL * 1000UL;  // v16: 2 นาที (เดิม 5) = เท่ากับหน้าต่างวัดความชัน STAB_SLOPE_WINDOW_MS
unsigned long measureStartMs = 0;

// ---------------------------------------------------------------------------------------------
// v15: ตัวตรวจ "แนวโน้มไหลช้า" (slow-trend gate) — แก้ปัญหากราฟ "นิ่งหลอก" (false plateau)
// ---------------------------------------------------------------------------------------------
// ที่มา: เมื่อกระโดดข้ามตัวอย่างที่ %RH ต่างกันมาก (เช่น วัด CaCl2 (~0.42) ต่อด้วยน้ำ (~1.0) แล้วย้อนกลับมา
// CaCl2 อีกที) เซนเซอร์ capacitive มักตอบสนองเป็น "สองช่วงเวลา" ซ้อนกัน: ช่วงแรกเปลี่ยนเร็ว ช่วงหลังไหลช้ามาก
// (โพลิเมอร์ดูดซับ/คายความชื้นตกค้างจากตัวอย่างก่อนหน้าช้ากว่าที่ post-heat 8 วิจะไล่ออกได้หมด) เกณฑ์ "นิ่ง" เดิม
// ดูแค่หน้าต่าง 5 นาทีล่าสุดว่าช่วงกว้าง <= SAVE_PROMPT_TOL หรือไม่ — ถ้าจังหวะนั้นเป็นรอยต่อระหว่างช่วงเร็ว
// (จบแล้ว) กับช่วงช้า (ยังไม่เริ่มขยับชัด) หน้าต่างจะดู "แบน" ทั้งที่ค่าจริงยังไหลต่ออีกหลายจุดสิบ ทำให้ล็อกค่าเร็ว
// เกินไป (เช่น น้ำได้แค่ ~0.65 ทั้งที่ควรจะ ~1.0) วิธีแก้: เก็บค่าเฉลี่ยเป็นบล็อกทุก TREND_SAMPLE_INTERVAL_MS
// คู่ขนานไปตลอดการวัด (ไม่ใช่แค่ตอนอยู่ในโหมด Predict) แล้วใช้ fitAR1() ตัวเดียวกับโหมด Predict ทำนายจุดสมดุล
// ระยะไกลจากข้อมูลบล็อกทั้งหมด — ถ้าจุดสมดุลที่ทำนายได้ยังห่างจากค่าเฉลี่ยหน้าต่างสั้นเกิน TREND_GATE_TOL แสดงว่า
// ยังมีแนวโน้มไหลช้าเหลืออยู่จริง จึง "ยับยั้ง" ไม่ให้ล็อกค่าว่านิ่ง แม้หน้าต่างสั้นจะแบนแล้วก็ตาม
#define TREND_MAX_SAMPLES 40
const unsigned long TREND_SAMPLE_INTERVAL_MS = 30000UL;  // 1 บล็อกทุก 30 วิ (ครอบคลุมได้ถึง 20 นาทีข้อมูลแนวโน้ม)
const int TREND_MIN_SAMPLES = 6;          // ต้องมีอย่างน้อย 6 บล็อก (~3 นาที) ก่อนเริ่มใช้ผลของตัวตรวจนี้ยับยั้งการล็อกค่า
const float TREND_GATE_TOL = 0.006f;      // จุดสมดุลระยะไกลกับค่าเฉลี่ยหน้าต่างสั้น ห่างกันเกินนี้ = ยังไหลอยู่จริง (ไม่ใช่แค่ noise)
float trendBuf[TREND_MAX_SAMPLES];
int trendN = 0;
double trendBlockSum = 0;
int trendBlockCnt = 0;
unsigned long trendBlockStartMs = 0;
bool trendStillDrifting = false;   // true = ตัวตรวจนี้กำลังยับยั้งการล็อก "นิ่ง" อยู่ (โชว์ไว้เผื่อ debug/System Health)

void resetTrendGate() {
  trendN = 0;
  trendBlockSum = 0;
  trendBlockCnt = 0;
  trendBlockStartMs = millis();
  trendStillDrifting = false;
}

// เรียกทุกรอบจาก updateStabilityWindow() ด้วยค่าดิบเดียวกัน — สะสมเป็นบล็อกแยกต่างหากจากหน้าต่างนิ่งระยะสั้น
// คืน true ถ้า "ยังไหลอยู่จริง" (ต้องยับยั้งไม่ให้ล็อกค่านิ่ง) false ถ้าไม่มีหลักฐานว่ายังไหล (ปล่อยผ่านตามเกณฑ์เดิม)
bool updateTrendGate(float v) {
  trendBlockSum += v;
  trendBlockCnt++;
  if (millis() - trendBlockStartMs >= TREND_SAMPLE_INTERVAL_MS) {
    float avg = (float)(trendBlockSum / trendBlockCnt);
    trendBlockSum = 0;
    trendBlockCnt = 0;
    trendBlockStartMs = millis();
    if (trendN >= TREND_MAX_SAMPLES) {
      for (int i = 0; i < TREND_MAX_SAMPLES - 1; i++) trendBuf[i] = trendBuf[i + 1];
      trendN = TREND_MAX_SAMPLES - 1;
    }
    trendBuf[trendN++] = avg;
  }
  if (trendN < TREND_MIN_SAMPLES) { trendStillDrifting = false; return false; }
  float eq, tau;
  if (!fitAR1(trendBuf, trendN, eq, tau)) { trendStillDrifting = false; return false; }  // ไม่มีแนวโน้มเอ็กซ์โพเนนเชียลที่ฟิตได้ -> ถือว่านิ่งจริง ปล่อยผ่าน
  float lastBlock = trendBuf[trendN - 1];
  trendStillDrifting = fabs(eq - lastBlock) > TREND_GATE_TOL;
  return trendStillDrifting;
}
// ---------------------------------------------------------------------------------------------

// ============================================================================
//  v16 (ขั้นที่ 1): ตัวตรวจ "ความนิ่ง" ตัวเดียว ใช้ร่วมกันทุกโหมด (วัดปกติ / เปรียบเทียบ / คาลิเบตจากเว็บ)
//  ผลลัพธ์ = phase 0/1/2 -> ไฟ LED สีเดียวกันหมด:
//     0 = แดง    ค่ายังเคลื่อนที่อยู่ (ยังไม่คงที่)
//     1 = เหลือง ค่าคงที่มาแล้วอย่างน้อย STAB_YELLOW_MS (20 วิ) ช่วงกว้าง <= STAB_TOL แต่ยังไม่ครบเกณฑ์ล็อก
//     2 = เขียว  ค่าคงที่ตลอดหน้าต่าง STAB_WINDOW_MS (60 วิ) และความชัน <= STAB_SLOPE_MAX_PER_MIN -> ล็อกค่าได้
//  หลักการ: เฉลี่ยค่าเป็นบล็อก 5 วิ (ตัด noise ต่อตัวอย่าง) แล้วดูค่า aw (หลังคาลิเบรต) ของบล็อก — ตามค่าที่โชว์บนจอ
//  ไม่ใช่ค่าดิบ เพราะตารางคาลิเบรตยืดสเกลได้ถึง ~1.4 เท่า จึงคุมเกณฑ์ที่หน่วย aw ให้ตรงตามที่ผู้ใช้ต้องการ (+-0.0005-0.001)
//  ความชันระยะ 2 นาที (aw/นาที) กันกรณี "นิ่งหลอก": ช่วงกว้าง 60 วิ แคบได้ทั้งที่ค่ายังไหลช้า ๆ เข้าหาสมดุล
//  (ค่าที่ยังขาดถึงสมดุล ~ ความชัน x tau) — จำลองแล้ว: ตัวอย่างนิ่งเร็ว (tau<=2 นาที) ล็อกได้ใน ~2-11 นาที ค่าคลาดจากสมดุล <0.0005
// ============================================================================
const float STAB_TOL = SAVE_PROMPT_TOL;                 // ช่วงกว้างสูงสุด (aw) ที่ยอมรับว่า "คงที่" — ปรับที่ SAVE_PROMPT_TOL (0.0005 เข้มขึ้น / 0.001 ง่ายขึ้น)
const float STAB_HYST_MULT = 1.5f;                      // ฮิสเทอรีซิส: เมื่อเขียวแล้ว ยอมให้แกว่งได้ TOL x 1.5 ก่อนหลุด (กันไฟกระพริบเขียว<->เหลืองที่ขอบ)
const float STAB_SLOPE_MAX_PER_MIN = 0.0002f;           // ความชันสูงสุด (aw/นาที) ที่ยอมรับว่านิ่ง (ยิ่งน้อยยิ่งแม่นแต่รอนานขึ้นกับตัวอย่างไหลช้า)
const unsigned long STAB_BLOCK_MS = 5000UL;             // 1 บล็อก = 5 วิ
const unsigned long STAB_YELLOW_MS = 20000UL;           // คงที่นาน 20 วิ = เหลือง (ตรงกับหน้าต่างป็อปอัปบนเว็บ STABLE_POPUP_WINDOW_MS)
const unsigned long STAB_SLOPE_WINDOW_MS = 480000UL;    // v23: 8 นาที (เดิม 2 นาที) — ต้อง >= STAB_WINDOW_MS เสมอ (ดู static_assert ด้านล่าง)
#define STAB_BLK_CAP 100                                // v23: เดิม 32 บล็อก (160 วิ) เล็กกว่า STAB_SLOPE_WINDOW_MS ใหม่ (480 วิ = 96 บล็อก) เผื่อไว้ 100
const int STAB_WIN_BLKS = STAB_WINDOW_MS / STAB_BLOCK_MS;          // 12
const int STAB_YELLOW_BLKS = STAB_YELLOW_MS / STAB_BLOCK_MS;       // 4
const int STAB_SLOPE_BLKS = STAB_SLOPE_WINDOW_MS / STAB_BLOCK_MS;  // 24
static_assert((STAB_WINDOW_MS / STAB_BLOCK_MS) <= (STAB_SLOPE_WINDOW_MS / STAB_BLOCK_MS), "slope window must cover the stable window");
static_assert((STAB_SLOPE_WINDOW_MS / STAB_BLOCK_MS) <= STAB_BLK_CAP, "STAB_BLK_CAP too small for the slope window");
// v23: AUTOCAL_GRAPH_CAP ถูก #define ไว้ก่อนหน้านี้มาก (สุ่ม ~1 ตัวอย่าง/วิ) — ต้องเก็บได้อย่างน้อย
// STAB_SLOPE_WINDOW_MS/1000 วิ ไม่งั้น minAgeOk ใน computeAutoCalHoldPhase() จะไม่มีวันเป็นจริง (บั๊กสายพันธุ์เดียวกับ
// STAB_BUF_CAP ที่เคยแก้ไปแล้วใน v12) เช็คไว้ตรงนี้กันคนแก้ STAB_SLOPE_WINDOW_MS ในอนาคตแล้วลืมตาม AUTOCAL_GRAPH_CAP
static_assert(AUTOCAL_GRAPH_CAP >= (int)(STAB_SLOPE_WINDOW_MS / 1000UL), "AUTOCAL_GRAPH_CAP must cover STAB_SLOPE_WINDOW_MS worth of ~1Hz samples");
// v23: AUTOCAL_WIN_SAMPLES (หน้าต่างเฉลี่ยค่าที่ล็อกตอนจบรอบคาลิเบตอัตโนมัติ) ตั้งใจให้เท่ากับ STAB_WINDOW_MS วินาที
// เสมอ (รายงานค่าเฉลี่ยของหน้าต่างเดียวกับที่ใช้ตัดสินว่า "นิ่ง") เช็คไว้กันคนแก้ STAB_WINDOW_MS แล้วลืมตาม
static_assert(AUTOCAL_WIN_SAMPLES == (int)(STAB_WINDOW_MS / 1000UL), "AUTOCAL_WIN_SAMPLES must match STAB_WINDOW_MS in seconds");

float stabBlkRaw[STAB_BLK_CAP];    // ค่าดิบเฉลี่ยต่อบล็อก (ใช้คำนวณค่า aw ที่ล็อก)
float stabBlkAw[STAB_BLK_CAP];     // aw (หลังคาลิเบรต) เฉลี่ยต่อบล็อก (ใช้ตัดสินความนิ่ง)
float stabBlkTemp[STAB_BLK_CAP];   // อุณหภูมิเฉลี่ยต่อบล็อก (NAN = อ่านไม่ได้)
float stabBlkPwm[STAB_BLK_CAP];    // v18: PWM (%) ที่ส่งออกจริง เฉลี่ยต่อบล็อก — ใช้ตัดสิน "PWM นิ่ง"
int stabBlkN = 0;
double stabBlkSumPwm = 0;
double stabBlkSumRaw = 0, stabBlkSumT = 0;
int stabBlkCnt = 0, stabBlkTCnt = 0;
unsigned long stabBlkStartMs = 0;
int stabPhase = 0;                 // 0/1/2 ตามด้านบน — โหมดวัด/เปรียบเทียบอ่านค่านี้ไปทำไฟ LED/สีบนจอ/เว็บ
float stabRangeAw = NAN;           // ช่วงกว้าง aw ในหน้าต่าง 60 วิ (debug)
float stabSlopePerMin = NAN;       // ความชัน aw/นาที ในหน้าต่าง 2 นาที (debug)

// ---------- v28: ตัวทดสอบความชัน "แบบ Bayesian ที่แยก ripple ของ PID ออกจากการไหลจริง" ----------
// เดิม: ความชัน = เส้นตรงกำลังสองน้อยสุด แล้วเทียบกับเกณฑ์ตรง ๆ (|b| <= slopeMax) — ไม่สนใจว่าค่าประมาณความชันมีความไม่แน่นอนเท่าไร
//       ที่ความชันจริง = เกณฑ์พอดี จึงผ่าน/ไม่ผ่านแบบเหรียญ (ผลจำลอง ~54% ผ่าน) และ ripple ของ PID (คาบ ~2 นาที) ปนเข้าค่าประมาณ
// ใหม่: ฟิตเชิงเส้นแบบมีเทอมคาบ  y = a + b*u + c*cos(2*pi*i/P) + d*sin(2*pi*i/P) สแกนคาบ P หาที่พอดีที่สุด (profile likelihood)
//       เลือกระหว่าง "เส้นตรงล้วน" กับ "เส้นตรง+ripple" ด้วย BIC (ปรับโทษเรื่องการสแกนคาบ) แล้วได้ posterior ของความชัน b ~ N(b_hat, sd^2)
//       (ไม่ใช้ตัวอย่างฝึก — เป็นสถิติล้วน) เส้นตรงล้วนแก้ความสัมพันธ์ของ residual ด้วยตัวคูณ sqrt((1+rho)/(1-rho))
//       ตัดสิน "ไม่ไหล" เมื่อ |b_hat| + Z*sd <= เกณฑ์ความชัน  (ขอบบนของช่วงเชื่อมั่นต้องอยู่ในเกณฑ์ ไม่ใช่แค่ค่าประมาณจุดเดียว)
// ผลจำลอง (หน้าต่าง 96 บล็อก, ripple คาบ ~115 วิ): ความชันจริง <= 0.0001 aw/นาที ผ่านทุกกรณี, >= 0.0003 ปฏิเสธทุกกรณี,
//       ความชันจริง = เกณฑ์พอดี (0.0002) ผ่านเพียง ~5% (เดิม ~54%) — ตั้ง STAB_BAYES_ENABLE 0 = พฤติกรรมเดิมทุกประการ
#define STAB_BAYES_ENABLE 1
const int   STAB_BAYES_WIN_BLKS = 96;     // หน้าต่างบล็อกที่ใช้ทดสอบความชัน: ต้อง >= STAB_WIN_BLKS (48) และ <= STAB_BLK_CAP; 96 = เท่าเดิม (8 นาที)
                                          // ผลจำลองบอกว่า 48-64 (4-5 นาที) ก็แยกได้ แต่ยังไม่ได้ทดสอบกับ ripple จริงของเครื่อง จึงคงค่าเดิมไว้ก่อน
const float STAB_BAYES_Z = 1.645f;        // ตัวคูณขอบบนช่วงเชื่อมั่น (~95% ด้านเดียว)
const int   STAB_BAYES_PMIN = 15, STAB_BAYES_PMAX = 40;   // คาบ ripple ที่สแกน (บล็อก 5 วิ) = 75-200 วิ
const double STAB_BAYES_SIGMA_FLOOR = 0.00003;            // noise ต่ำสุดของค่าเฉลี่ยบล็อก (aw) กัน log(0)/ความมั่นใจเกินจริงเมื่อข้อมูลเรียบมาก
static_assert(STAB_BAYES_WIN_BLKS >= (STAB_WINDOW_MS / STAB_BLOCK_MS), "Bayes slope window must cover the stable window");
static_assert(STAB_BAYES_WIN_BLKS <= STAB_BLK_CAP, "Bayes slope window exceeds STAB_BLK_CAP");
float stabBayesSdPerMin = NAN;    // sd ของความชัน (aw/นาที) จากรอบตัดสินล่าสุด (debug)
float stabBayesConfPct = NAN;     // ความน่าจะเป็น (%) ว่าความชันจริงอยู่ในเกณฑ์ (debug)
bool  stabBayesHarmonic = false;  // true = พบ ripple แบบคาบและถูกแยกออกแล้ว

struct BayesSlope { float slopePerMin, sdPerMin; bool harmonic; };

// ฟิตหนึ่งโมเดล (P<=0 เส้นตรง / P>0 เส้นตรง+คาบ P บล็อก) คืน slope/บล็อก, C11 = [(X'X)^-1]_{slope,slope}, SSE, ρ(lag-1) ของ residual
static bool bsFit(const float* y, int m, int P, double& slope, double& c11, double& sse, double& rho) {
  const int p = (P > 0) ? 4 : 2;
  double S[4][6];
  for (int a = 0; a < 4; a++) for (int b = 0; b < 6; b++) S[a][b] = 0;
  const double mid = (m - 1) * 0.5, y0 = y[0];
  double cw = 1, sw = 0, cwStep = 1, swStep = 0;
  if (P > 0) { cwStep = cos(6.283185307179586 / P); swStep = sin(6.283185307179586 / P); }
  double c = 1, sn = 0;   // cos/sin(2*pi*i/P) ด้วยการหมุนทีละก้าว (เลี่ยง trig ต่อจุด)
  for (int i = 0; i < m; i++) {
    double x[4] = { 1.0, i - mid, c, sn };
    double d = (double)y[i] - y0;
    for (int a = 0; a < p; a++) { for (int b = 0; b < p; b++) S[a][b] += x[a] * x[b]; S[a][p] += x[a] * d; }
    double nc = c * cwStep - sn * swStep; sn = sn * cwStep + c * swStep; c = nc;
  }
  (void)cw; (void)sw;
  S[1][p + 1] = 1.0;   // คอลัมน์ที่สอง = e1 -> ได้ [(X'X)^-1]_{.,1}
  const int cols = p + 2;
  for (int col = 0; col < p; col++) {
    int piv = col;
    for (int r = col + 1; r < p; r++) if (fabs(S[r][col]) > fabs(S[piv][col])) piv = r;
    if (fabs(S[piv][col]) < 1e-12) return false;
    if (piv != col) for (int k = 0; k < cols; k++) { double t = S[col][k]; S[col][k] = S[piv][k]; S[piv][k] = t; }
    for (int r = col + 1; r < p; r++) {
      double f = S[r][col] / S[col][col];
      for (int k = col; k < cols; k++) S[r][k] -= f * S[col][k];
    }
  }
  double beta[4] = {0, 0, 0, 0}, inv1[4] = {0, 0, 0, 0};
  for (int r = p - 1; r >= 0; r--) {
    double v1 = S[r][p], v2 = S[r][p + 1];
    for (int k = r + 1; k < p; k++) { v1 -= S[r][k] * beta[k]; v2 -= S[r][k] * inv1[k]; }
    beta[r] = v1 / S[r][r]; inv1[r] = v2 / S[r][r];
  }
  slope = beta[1]; c11 = inv1[1];
  // residual: SSE และ lag-1 autocorrelation
  sse = 0; double sl = 0, prev = 0; c = 1; sn = 0;
  for (int i = 0; i < m; i++) {
    double fit = beta[0] + beta[1] * (i - mid) + ((P > 0) ? beta[2] * c + beta[3] * sn : 0.0);
    double r = ((double)y[i] - y0) - fit;
    sse += r * r; if (i > 0) sl += r * prev; prev = r;
    double nc = c * cwStep - sn * swStep; sn = sn * cwStep + c * swStep; c = nc;
  }
  rho = (sse > 1e-18) ? sl / sse : 0.0;
  return true;
}

// คืน posterior ของความชัน (aw/นาที) ของ y[0..m-1] (บล็อก 5 วิเรียงเก่า->ใหม่) — ผลลัพธ์ถูกแคชตามเนื้อข้อมูล (คำนวณใหม่เมื่อมีบล็อกใหม่เท่านั้น)
bool bayesSlope(const float* y, int m, BayesSlope& out) {
  static int cM = 0; static double cSum = 0; static float cLast = 0; static BayesSlope cRes; static bool cOk = false;
  double sum = 0; for (int i = 0; i < m; i++) sum += y[i];
  if (m == cM && sum == cSum && y[m - 1] == cLast) { out = cRes; return cOk; }
  cM = m; cSum = sum; cLast = y[m - 1]; cOk = false;
  if (m < 12) return false;
  const double perMin = 60000.0 / (double)STAB_BLOCK_MS;
  const double sig2min = STAB_BAYES_SIGMA_FLOOR * STAB_BAYES_SIGMA_FLOOR;

  double b0, c0, sse0, rho0;
  if (!bsFit(y, m, 0, b0, c0, sse0, rho0)) return false;
  double sseF0 = fmax(sse0, m * sig2min);
  double r0 = rho0 < 0 ? 0 : (rho0 > 0.9 ? 0.9 : rho0);
  double s0 = sqrt(sseF0 / (m - 2));
  double sd0 = s0 * sqrt(c0) * sqrt((1.0 + r0) / (1.0 - r0));    // เส้นตรงล้วน: ปรับตามความสัมพันธ์ของ residual

  // สแกนคาบ: ก้าวละ 2 บล็อกก่อน แล้วละเอียดรอบค่าที่ดีสุด
  int pMax = STAB_BAYES_PMAX; if (pMax > m / 2) pMax = m / 2;
  double bestSse = 1e300, bestB = 0, bestC = 0; int bestP = 0;
  for (int P = STAB_BAYES_PMIN; P <= pMax; P += 2) {
    double b, c11, sse, rho;
    if (bsFit(y, m, P, b, c11, sse, rho) && sse < bestSse) { bestSse = sse; bestB = b; bestC = c11; bestP = P; }
  }
  if (bestP > 0) {
    for (int P = bestP - 1; P <= bestP + 1; P += 2) {
      if (P < STAB_BAYES_PMIN || P > pMax) continue;
      double b, c11, sse, rho;
      if (bsFit(y, m, P, b, c11, sse, rho) && sse < bestSse) { bestSse = sse; bestB = b; bestC = c11; bestP = P; }
    }
  }
  bool useH = false; double bS = b0, sdS = sd0;
  if (bestP > 0 && m > 8) {
    double sseF1 = fmax(bestSse, m * sig2min);
    double bic0 = m * log(sseF0 / m) + 2.0 * log((double)m);
    double bic1 = m * log(sseF1 / m) + 6.0 * log((double)m);     // 4 พารามิเตอร์ + โทษการสแกนคาบ
    if (bic1 < bic0) {
      useH = true; bS = bestB;
      sdS = sqrt(sseF1 / (m - 6)) * sqrt(bestC);
    }
  }
  cRes.slopePerMin = (float)(bS * perMin);
  cRes.sdPerMin = (float)(sdS * perMin);
  cRes.harmonic = useH;
  cOk = true;
  out = cRes;
  return true;
}

// คืน true ถ้าค่าใน x[0..n-1] "คงที่": ช่วงกว้าง <= tol และครึ่งหลังไม่ต่างจากครึ่งแรกเกิน tol/2 (ไม่ไหลเป็นแนวโน้ม)
// v21: pwmSteady = PWM เทลเทียร์นิ่งตลอดหน้าต่างนี้ (ตรวจเจอ "ทุกการตรวจ" ไม่ใช่แค่บางครั้ง) — ถ้าใช่ แปลว่าค่าที่ขึ้น-ลง
// เป็นแค่ ripple ของลูปควบคุมเอง ไม่ใช่ยังไม่นิ่งจริง จึง "นับว่าคงที่" โดยไม่จำกัดช่วงกว้าง (mx-mn) อีกต่อไป — ยังคงกัน
// แนวโน้มไหลจริงที่อาจแอบซ่อนอยู่ใต้ ripple ด้วยเกณฑ์ครึ่งแรก-ครึ่งหลัง (ผ่อนขึ้นเล็กน้อยตาม STAB_PWM_AW_TOL_MULT/RIPPLE)
bool stabFlat(const float* x, int n, float tol, bool pwmSteady) {
  if (n < 2) return false;
  float effTol = tol;
  if (pwmSteady) {
    float byMult = tol * STAB_PWM_AW_TOL_MULT, byAdd = tol + STAB_PWM_AW_RIPPLE;
    effTol = (byMult > byAdd) ? byMult : byAdd;
  } else {
    float mn = x[0], mx = x[0];
    for (int i = 1; i < n; i++) { if (x[i] < mn) mn = x[i]; if (x[i] > mx) mx = x[i]; }
    if ((mx - mn) > tol) return false;
  }
  int h = n / 2;
  float a = 0, b = 0;
  for (int i = 0; i < h; i++) { a += x[i]; b += x[n - h + i]; }
  return fabsf((b - a) / h) <= effTol * 0.5f;
}

// v18: PWM ของ nBlk บล็อกล่าสุด "นิ่ง" หรือไม่ — ลูปควบคุมเข้าสมดุล: PWM เฉลี่ยอยู่ในช่วงทำงาน (ไม่ปิด/ไม่เต็มกำลัง)
// และแกว่งระหว่างบล็อกไม่เกิน STAB_PWM_RANGE_TOL_PCT ; pwm == NULL หรือมีบล็อกไม่ครบ = ไม่นิ่ง (ใช้เกณฑ์เข้มเดิม)
bool stabPwmSteady(const float* pwm, int n, int nBlk) {
  if (!pwm || n < nBlk || nBlk < 2) return false;
  float mn = pwm[n - nBlk], mx = mn, sum = 0;
  for (int i = n - nBlk; i < n; i++) {
    if (pwm[i] < mn) mn = pwm[i];
    if (pwm[i] > mx) mx = pwm[i];
    sum += pwm[i];
  }
  float mean = sum / nBlk;
  if (mean < STAB_PWM_MIN_PCT || mean > STAB_PWM_MAX_PCT) return false;
  return (mx - mn) <= STAB_PWM_RANGE_TOL_PCT;
}

// ฟังก์ชันตัดสินล้วน ๆ (ไม่แตะตัวแปรสถานะ) ใช้ร่วมกันระหว่างตัวตรวจของโหมดวัดกับตัวตรวจ LED ของโหมดคาลิเบตจากเว็บ
//   blk[] = ค่า aw เฉลี่ยรายบล็อก 5 วิ เรียงเก่า -> ใหม่ (n บล็อก)  minAgeOk = วัดมานานพอแล้ว  relaxed = เคยเขียวอยู่ (ใช้ฮิสเทอรีซิส)
//   v18: pwmBlk[] = PWM (%) เฉลี่ยรายบล็อก ยาว n บล็อกเท่ากับ blk[] (NULL = ไม่มีข้อมูล PWM -> ใช้เกณฑ์เข้มเดิม)
//   v21: ถ้า PWM นิ่งตลอดหน้าต่างที่เกี่ยวข้อง (ตรวจเจอทุกบล็อก ไม่ใช่แค่บางครั้ง) = ลูปควบคุมเข้าสมดุลแล้ว จะไม่จำกัด
//        ช่วงกว้าง aw อีกต่อไป (ปล่อยผ่านเสมอใน stabFlat()) เพราะค่าที่ขึ้น-ลงเป็นแค่ ripple ของ PID ไม่ใช่ยังไม่นิ่งจริง
//        ยังกันแนวโน้มไหลจริงที่ซ่อนอยู่ใต้ ripple ด้วยเกณฑ์ครึ่งแรก-ครึ่งหลังในตัว stabFlat() + ความชัน 2 นาทีด้านล่าง
int stabClassifyBlocks(const float* blk, int n, bool minAgeOk, bool relaxed, const float* pwmBlk, float* rangeOut, float* slopeOut) {
  float f = relaxed ? STAB_HYST_MULT : 1.0f;
  float tol = STAB_TOL * f;
  int phase = 0;
  bool pwmY = stabPwmSteady(pwmBlk, n, STAB_YELLOW_BLKS);
  bool pwmW = stabPwmSteady(pwmBlk, n, STAB_WIN_BLKS);
#if STAB_BAYES_ENABLE
  const int slopeBlks = STAB_BAYES_WIN_BLKS;    // v28: หน้าต่างทดสอบความชัน (ค่าเริ่มต้น 96 = เท่าเดิม)
#else
  const int slopeBlks = STAB_SLOPE_BLKS;
#endif
  bool pwmS = stabPwmSteady(pwmBlk, n, slopeBlks);
  if (rangeOut) *rangeOut = NAN;
  if (slopeOut) *slopeOut = NAN;
  if (n >= STAB_YELLOW_BLKS && stabFlat(blk + n - STAB_YELLOW_BLKS, STAB_YELLOW_BLKS, tol, pwmY)) phase = 1;
  if (n >= slopeBlks) {
    const float* w = blk + n - STAB_WIN_BLKS;
    float mn = w[0], mx = w[0];
    for (int i = 1; i < STAB_WIN_BLKS; i++) { if (w[i] < mn) mn = w[i]; if (w[i] > mx) mx = w[i]; }
    if (rangeOut) *rangeOut = mx - mn;
    // ความชันกำลังสองน้อยสุดของ STAB_SLOPE_BLKS บล็อกล่าสุด (aw/นาที) — ลบค่าแรกออกก่อนคำนวณกันเลขทศนิยมคลาด
    const float* s = blk + n - slopeBlks;
    int m = slopeBlks;
    float ybar = 0;
    for (int i = 0; i < m; i++) ybar += (s[i] - s[0]);
    ybar /= m;
    float xbar = (m - 1) * 0.5f, sxy = 0, sxx = 0;
    for (int i = 0; i < m; i++) { float dx = i - xbar; sxy += dx * ((s[i] - s[0]) - ybar); sxx += dx * dx; }
    float slopePerMin = (sxy / sxx) * (60000.0f / (float)STAB_BLOCK_MS);
    if (slopeOut) *slopeOut = slopePerMin;
    float slopeMax = STAB_SLOPE_MAX_PER_MIN * f * (pwmS ? STAB_PWM_SLOPE_MULT : 1.0f);
    bool slopeOk = fabsf(slopePerMin) <= slopeMax;
#if STAB_BAYES_ENABLE
    {
      // v28: ตัดสินด้วยขอบบนของช่วงเชื่อมั่นของความชันที่แยก ripple ออกแล้ว (ไม่ใช่ค่าประมาณจุดเดียว)
      BayesSlope bs;
      if (bayesSlope(s, m, bs)) {
        slopePerMin = bs.slopePerMin;
        if (slopeOut) *slopeOut = slopePerMin;
        slopeOk = (fabsf(bs.slopePerMin) + STAB_BAYES_Z * bs.sdPerMin) <= slopeMax;
        stabBayesSdPerMin = bs.sdPerMin;
        stabBayesHarmonic = bs.harmonic;
        float sd = bs.sdPerMin > 1e-9f ? bs.sdPerMin : 1e-9f;
        float pHi = 0.5f * (1.0f + erff((slopeMax - bs.slopePerMin) / (sd * 1.41421356f)));
        float pLo = 0.5f * (1.0f + erff((-slopeMax - bs.slopePerMin) / (sd * 1.41421356f)));
        stabBayesConfPct = 100.0f * (pHi - pLo);
      }
    }
#endif
    if (minAgeOk && stabFlat(w, STAB_WIN_BLKS, tol, pwmW) && slopeOk) phase = 2;
  }
  return phase;
}

// อุณหภูมิของ nBlk บล็อกล่าสุดนิ่งพอหรือไม่ (อ่านอุณหภูมิไม่ได้ = ไม่บล็อกการวัด เหมือนเดิม)
// v18: pwmSteady = PWM นิ่งตลอดหน้าต่างเดียวกัน -> ยอมให้อุณหภูมิแกว่งได้ STAB_PWM_TEMP_TOL_MULT เท่า (เป็น ripple ของการควบคุม)
bool stabTempOk(int nBlk, bool pwmSteady) {
  if (stabBlkN < nBlk) return false;
  float mn = 1e9f, mx = -1e9f;
  for (int i = stabBlkN - nBlk; i < stabBlkN; i++) {
    if (isnan(stabBlkTemp[i])) return true;
    if (stabBlkTemp[i] < mn) mn = stabBlkTemp[i];
    if (stabBlkTemp[i] > mx) mx = stabBlkTemp[i];
  }
  return (mx - mn) <= (pwmSteady ? STAB_TEMP_RANGE_TOL_C * STAB_PWM_TEMP_TOL_MULT : STAB_TEMP_RANGE_TOL_C);
}

void resetStabilityWindow() {
  stabBufCount = 0;
  stabBufHead = 0;
  stabTempBufCount = 0;
  stabTempBufHead = 0;
  measureStable = false;
  measureStartMs = millis();
  resetTrendGate();
  stabBlkN = 0; stabBlkSumRaw = stabBlkSumT = 0; stabBlkCnt = stabBlkTCnt = 0;   // v16: ล้างบล็อกของตัวตรวจนิ่งด้วย
  stabBlkSumPwm = 0; stabPwmSteadyNow = false;                                    // v18: ล้างสถานะ PWM นิ่งด้วย
  stabBlkStartMs = millis();
  stabPhase = 0; stabRangeAw = stabSlopePerMin = NAN;
}

void getStabRange(float& mn, float& mx) {
  mn = stabBuf[0];
  mx = stabBuf[0];
  for (int i = 0; i < stabBufCount; i++) {
    if (stabBuf[i] < mn) mn = stabBuf[i];
    if (stabBuf[i] > mx) mx = stabBuf[i];
  }
}

void getStabTempRange(float& mn, float& mx) {
  mn = stabTempBuf[0];
  mx = stabTempBuf[0];
  for (int i = 0; i < stabTempBufCount; i++) {
    if (stabTempBuf[i] < mn) mn = stabTempBuf[i];
    if (stabTempBuf[i] > mx) mx = stabTempBuf[i];
  }
}

// v = ค่าดิบ (raw, ก่อนคาลิเบรต) ; t = อุณหภูมิ °C ในจังหวะเดียวกัน
// v16: เรียกทุกรอบวัด (~100 ms) — สะสมเป็นบล็อกตามเวลาจริง (STAB_BLOCK_MS) จึงไม่ขึ้นกับความเร็วลูป แล้วให้ stabClassifyBlocks()
// ตัดสิน phase (ดูคำอธิบายด้านบน) อุณหภูมิต้องนิ่งพร้อมกันเสมอ (กันล็อกค่าปลอมตอนเทลเทียร์ยังไล่อุณหภูมิ) ผลลัพธ์อยู่ใน stabPhase / measureStable
void updateStabilityWindow(float v, float t) {
  unsigned long now = millis();

  // ring ตัวอย่างดิบ: ใช้เฉพาะเตือน "Sensor noise?" (getStabRange) ไม่ได้ใช้ตัดสินความนิ่งอีกต่อไป
  stabBuf[stabBufHead] = v;
  stabBufHead = (stabBufHead + 1) % STAB_BUF_CAP;
  if (stabBufCount < STAB_BUF_CAP) stabBufCount++;
  if (!isnan(t)) {
    stabTempBuf[stabTempBufHead] = t;
    stabTempBufHead = (stabTempBufHead + 1) % STAB_BUF_CAP;
    if (stabTempBufCount < STAB_BUF_CAP) stabTempBufCount++;
  }

  // สะสมบล็อก
  stabBlkSumRaw += v; stabBlkCnt++;
  stabBlkSumPwm += peltierAppliedF * 100.0f / 255.0f;   // v18: PWM ที่ส่งออกจริง (%) ณ จังหวะเดียวกับค่าอ่าน
  if (!isnan(t)) { stabBlkSumT += t; stabBlkTCnt++; }
  if (now - stabBlkStartMs >= STAB_BLOCK_MS && stabBlkCnt > 0) {
    float rawMean = (float)(stabBlkSumRaw / stabBlkCnt);
    float tMean = stabBlkTCnt ? (float)(stabBlkSumT / stabBlkTCnt) : NAN;
    if (stabBlkN >= STAB_BLK_CAP) {
      for (int i = 1; i < STAB_BLK_CAP; i++) { stabBlkRaw[i - 1] = stabBlkRaw[i]; stabBlkAw[i - 1] = stabBlkAw[i]; stabBlkTemp[i - 1] = stabBlkTemp[i]; stabBlkPwm[i - 1] = stabBlkPwm[i]; }
      stabBlkN = STAB_BLK_CAP - 1;
    }
    stabBlkRaw[stabBlkN] = rawMean;
    stabBlkTemp[stabBlkN] = tMean;
    stabBlkAw[stabBlkN] = applyCal(rawMean, isnan(tMean) ? currentTempC : tMean);
    stabBlkPwm[stabBlkN] = (float)(stabBlkSumPwm / stabBlkCnt);
    stabBlkN++;
    stabBlkSumRaw = 0; stabBlkSumT = 0; stabBlkCnt = 0; stabBlkTCnt = 0; stabBlkSumPwm = 0;
    stabBlkStartMs = now;
  }

  // ตัวตรวจแนวโน้มไหลช้าแบบ AR(1) เดิม: ยังคำนวณเพื่อแสดงสถานะ (stillDrifting ใน /data) แต่ไม่ใช้ยับยั้งการล็อกแล้ว —
  // v16 ใช้ความชันกำลังสองน้อยสุด 2 นาทีใน stabClassifyBlocks() แทน เพราะ AR(1) บนบล็อกสั้นไม่เสถียรเมื่อ tau ยาว (จำลองแล้ว
  // ค่า eq ที่ extrapolate คลาดได้หลายเท่าจาก noise) ทำให้ยับยั้งผิดพลาดได้
  updateTrendGate(v);

  bool minAgeOk = (now - measureStartMs) >= MIN_MEASURE_DURATION_MS;
  int ph = stabClassifyBlocks(stabBlkAw, stabBlkN, minAgeOk, stabPhase == 2, stabBlkPwm, &stabRangeAw, &stabSlopePerMin);
  // v18: อุณหภูมิต้องนิ่งพร้อมกันเสมอ แต่ถ้า PWM นิ่งตลอดหน้าต่าง (ลูปควบคุมเข้าสมดุล) ยอมให้แกว่งได้กว้างขึ้น
  bool pwmWin = stabPwmSteady(stabBlkPwm, stabBlkN, STAB_WIN_BLKS);
  bool pwmYel = stabPwmSteady(stabBlkPwm, stabBlkN, STAB_YELLOW_BLKS);
  stabPwmSteadyNow = pwmWin;
  if (ph == 2 && !stabTempOk(STAB_WIN_BLKS, pwmWin)) ph = 1;
  if (ph == 1 && !stabTempOk(STAB_YELLOW_BLKS, pwmYel)) ph = 0;

  bool wasStable = measureStable;
  stabPhase = ph;
  measureStable = (ph == 2);
  if (measureStable && !wasStable) {
    float rawLocked;
    if (pwmWin) {
      // v21: PWM นิ่งตลอดหน้าต่างนี้ -> ค่าที่ขึ้น-ลงเป็นแค่ ripple ของลูปควบคุม ใช้ค่าเฉลี่ยของจุดสูงสุด-ต่ำสุดของ
      // การแกว่งแทนค่าเฉลี่ยเลขคณิตธรรมดา (แม่นกว่า เพราะหน้าต่างล็อกมักไม่ครบพอดีจำนวนรอบ ripple เต็ม ๆ ซึ่งจะทำให้
      // ค่าเฉลี่ยธรรมดาเอียงออกจากจุดกึ่งกลางจริงของการแกว่งได้)
      float mn = stabBlkRaw[stabBlkN - STAB_WIN_BLKS], mx = mn;
      for (int i = stabBlkN - STAB_WIN_BLKS; i < stabBlkN; i++) {
        if (stabBlkRaw[i] < mn) mn = stabBlkRaw[i];
        if (stabBlkRaw[i] > mx) mx = stabBlkRaw[i];
      }
      rawLocked = (mn + mx) / 2.0f;
    } else {
      float sum = 0;
      for (int i = stabBlkN - STAB_WIN_BLKS; i < stabBlkN; i++) sum += stabBlkRaw[i];
      // เฉลี่ยค่าดิบในหน้าต่างนิ่ง แล้วค่อยแปลงผ่านสมการคาลิเบรตเป็นค่า aw สุดท้าย
      rawLocked = sum / STAB_WIN_BLKS;
    }
    measureFinalAw = applyCal(rawLocked, currentTempC);
#if !AW_RAW_MODE
    // v-substance-guess: ค่าที่ล็อกไว้ก็ควรตรงกับที่เดาไว้ตลอดการวัด ไม่ใช่แค่ค่าที่โชว์สด ๆ บนเว็บ
    measureFinalAw = awShownAtLock(measureFinalAw, rawLocked);   // v29: offset เดียวกับที่แสดงอยู่ (ไม่ขึ้นกับว่ามีเว็บโพลหรือไม่)
#endif
    measureTempWasStableToo = true;
    measureFinalDurationSec = (now - measureStartMs) / 1000UL;   // จับเวลาที่ใช้วัดจริง ณ จังหวะล็อกค่า
  }
}

bool iconVisible = false;
unsigned long iconShownAt = 0;
int iconX, iconY, activeIconType = 0;
const unsigned long iconShowDuration = 4000;
const int ICON_BASE_RADIUS = 7, ICON_MAX_RADIUS = 9;

const FoodCategory foodCategories[] = {
  { 0.00, 0.20, "Sugar / Salt / Milk powder", TFT_SKYBLUE },
  { 0.20, 0.60, "Noodle / Honey / Chocolate", TFT_CYAN },
  { 0.60, 0.85, "Jam / Jelly / Dried fruit / Nuts", TFT_GREENYELLOW },
  { 0.85, 0.93, "Dried meat / Condensed milk", TFT_YELLOW },
  { 0.93, 0.98, "Evap milk / Bread / Sausage", TFT_ORANGE },
  { 0.98, 1.01, "Fresh food / Meat / Veg / Milk", TFT_RED },
};
const int NUM_CATEGORIES = sizeof(foodCategories) / sizeof(foodCategories[0]);
const int iconForCategory[NUM_CATEGORIES] = { 0, 0, 1, 2, 2, 3 };

// ---------------- โหมด "ทำนายค่า aw สมดุลล่วงหน้า" (แยกจากการวัดปกติโดยสิ้นเชิง) — v13 เขียนสมการทำนายใหม่ ----------------
// หลักการ: ระหว่างที่ความชื้นเข้าสู่จุดสมดุล ค่าที่อ่านมักเป็นเอ็กซ์โพเนนเชียล v(t) = v_eq - A*exp(-t/tau)
// เมื่อสุ่มตัวอย่างห่างเท่ากัน (ทุก dt) จะได้ความสัมพันธ์เชิงเส้น  y[n+1] = a + k*y[n]  โดย k = exp(-dt/tau)
// และจุดสมดุล v_eq = a / (1 - k)
// สูตร 3 จุด Aitken เดิม: ใช้ค่าอ่านเดี่ยว ๆ 3 ค่า -> noise เล็กน้อยถูกขยายด้วยตัวหารที่เกือบเป็นศูนย์ ทำให้ค่าทำนายแกว่งแรง
// (จากการจำลอง error เฉลี่ย 0.01-0.1 aw) — สมการใหม่แก้ 4 จุด:
//   1) เฉลี่ยค่าอ่านทั้งช่วง (ราว 110 ค่า) เป็น 1 ตัวอย่างต่อ PRED_SAMPLE_INTERVAL_MS แทนการใช้ค่าอ่านเดี่ยว -> noise ลดราว 10 เท่า
//   2) ฟังก์ชันกำลังสองน้อยสุด (least squares) กับตัวอย่างล่าสุดสูงสุด PRED_MAX_SAMPLES ตัว แทนการใช้แค่ 3 จุด
//   3) มี AR(2) (เอ็กซ์โพเนนเชียลสองตัวซ้อนกัน) คู่กับ AR(1) — จับกรณีเส้นโค้งจริงที่มีช่วงเร็ว+ช่วงช้าได้
//   4) ตัวบอกความมั่นใจ: เทียบผลของ 3 วิธี ถ้าต่างกันเกิน PRED_CONFIRM_TOL แสดงเป็น "~" (ยังไม่มั่นใจ) และถ้ากราฟแบนแล้ว
//      (ช่วงกว้าง < PRED_FLAT_RANGE) ใช้ค่าเฉลี่ยตรง ๆ แทน (กันสมการเดาสุ่มตอนไม่มีแนวโน้มเหลือ)
// v15: ปรับจูนให้ทำนายละเอียด/แม่นขึ้นอีกขั้น โดยไม่แลกกับความไว (ยัง noise-robust เท่าเดิม):
//   5) หน้าต่างฟิตยาวขึ้น (PRED_MAX_SAMPLES 12 -> 20) และหน้าต่างฟิตช่วงหาง AR(1) ยาวขึ้น (8 -> 12 ตัวอย่างล่าสุด)
//      -> ข้อมูลเข้าสมการมากขึ้น ลด variance ของสัมประสิทธิ์ที่ fit ได้
//   6) fitAR1() เปลี่ยนจาก least squares ธรรมดา เป็นแบบถ่วงน้ำหนักตามความใหม่ (recency-weighted, ถ่วงด้วย
//      PRED_RECENCY_DECAY) — จุดข้อมูลล่าสุดมีน้ำหนักมากกว่าจุดเก่า เพราะช่วงท้ายเป็นตัวแทนพลวัตใกล้จุดสมดุลจริง
//      ได้ดีกว่าช่วงต้นที่โค้งอาจยังไม่เข้ารูปเอ็กซ์โพเนนเชียลเดียว (เช่น ช่วงฮีต/รอเย็นตกค้างเล็กน้อย)
//   7) รวมผลจาก 3 วิธี (AR2 / AR1 เต็มหน้าต่าง / AR1 หาง) ด้วยค่าเฉลี่ยถ่วงน้ำหนักแทนการเลือกวิธีเดียวเด็ดขาด
//      (เดิมเลือก AR2 ก่อนเสมอถ้ามี) -> ค่าทำนายเปลี่ยนแปลงราบรื่นขึ้นระหว่างบล็อก ไม่กระโดดเวลาแค่วิธีเดียวขยับ
//   8) เกณฑ์ตัดสิน "แบนแล้ว"/"มั่นใจ" แคบลงเล็กน้อย (PRED_FLAT_RANGE, PRED_CONFIRM_TOL) ให้สอดคล้องกับ noise
//      ที่ลดลงจากข้อ 5-6 (ยังคงห่างจาก SAVE_PROMPT_TOL/2 พอสมควร กันตัดสินไวเกินจากค่าที่ยังไม่นิ่งจริง)
#define PRED_MAX_SAMPLES 20
#define PRED_TAIL_WINDOW 12                      // v15: ขยายจาก 8 -> 12 ตัวอย่างล่าสุด สำหรับฟิตหาง AR(1)
const unsigned long PRED_SAMPLE_INTERVAL_MS = 15000UL;  // 1 ตัวอย่าง (เฉลี่ยบล็อก) ทุก 15 วินาที — ตัวอย่างแรกที่ใช้ทำนายได้ที่ ~75 วินาที
const int PRED_MIN_SAMPLES = 5;      // ต้องมีอย่างน้อยกี่ตัวอย่างก่อนเริ่มทำนาย
const float PRED_FLAT_RANGE = 0.0012f;   // ช่วงกว้างของตัวอย่างในหน้าต่างที่ต่ำกว่านี้ = ถึงสมดุลแล้ว (เขียว)
const float PRED_CONFIRM_TOL = 0.003f;   // 3 วิธีต่างกันไม่เกินนี้ = มั่นใจ (ไม่มี "~")
const float PRED_ETA_BAND = 0.002f;      // ETA = เวลาที่คาดว่าจะเข้าใกล้จุดสมดุลในระยะ +-นี้
const float PRED_RECENCY_DECAY = 0.90f;  // v15: น้ำหนักตัวอย่าง i (0=เก่าสุดในหน้าต่าง) = decay^(m-1-i) ในการฟิต fitAR1()
float predBuf[PRED_MAX_SAMPLES];         // ตัวอย่างค่าดิบ (เศษส่วน RH) แบบเฉลี่ยบล็อก เรียงจากเก่าไปใหม่
int predN = 0;
double predBlockSum = 0;                 // ผลรวมค่าอ่านในบล็อกปัจจุบัน
int predBlockCnt = 0;
unsigned long predBlockStartMs = 0;
unsigned long predStartMs = 0;
float predictedEqRaw = NAN;              // ค่าดิบสมดุลที่ทำนายได้ (NAN = ยังทำนายไม่ได้)
float predictedEqAw = NAN;               // ค่า aw สมดุลที่ทำนายได้ (ผ่านสมการคาลิเบรตแล้ว)
float predictedTauSec = NAN;             // ค่าคงที่เวลา (วินาที) ใช้ประมาณ ETA และวาดเส้นโค้งประ
bool predFlat = false;                   // true = กราฟแบนแล้ว (ถึงสมดุล)
bool predConfident = false;              // true = วิธีต่าง ๆ ให้ผลตรงกัน
int predInvalidStreak = 0;
unsigned long predLastUpdate = 0;        // จังหวะรีเฟรชหน้าจอ (ใช้ updateInterval เดียวกับโหมดวัดปกติ)
// กราฟของโหมด Predict: เก็บประวัติแบบบีบอัดอัตโนมัติ (ครบเต็มแล้วรวมทีละ 2 จุด) เพื่อให้เห็นทั้งรอบการวัดบนจอ + เผื่อพื้นที่ขวา 1/3 วาดเส้นประที่ทำนาย
float pgHist[MAX_POINTS];
int pgCount = 0, pgCap = 100;            // จำนวนจุดประวัติ / ความจุ (ส่วนซ้ายของกราฟ)
unsigned long pgStepMs = 500;            // เวลาต่อ 1 จุดประวัติ (เพิ่มเป็น 2 เท่าทุกครั้งที่บีบอัด)
unsigned long pgSlotStartMs = 0;
float pgSlotSum = 0;
int pgSlotN = 0;
float pgYmin = 0, pgYmax = 1;            // สเกลแกน Y ที่ซูมอัตโนมัติ
bool pgScaleInit = false;
int pgLabelTopI = -1, pgLabelBotI = -1;

// ---------------- โหมด "เปรียบเทียบ 2 ตัวอย่าง" (แยกจากการวัดปกติและโหมดทำนาย) ----------------
// วัดตัวอย่าง A ให้นิ่งจริงก่อน (ใช้ตรรกะ "นิ่ง" เดียวกับการวัดปกติทุกประการ) แล้วต่อด้วยตัวอย่าง B
// ทันทีในหน้าจอเดียวกัน จบแล้วโชว์ผลสรุปเทียบกันทั้งค่า aw และหมวดอาหารที่จัดอยู่ พร้อมตัวเลือกบันทึก
// ทั้งคู่ลงประวัติในครั้งเดียว — ทำได้ครบในตัวเครื่องโดยไม่ต้องพึ่งคอมพิวเตอร์/เว็บเลย
int compareStage = 0;         // 0 = กำลังวัดตัวอย่าง A, 1 = กำลังวัดตัวอย่าง B
float compareAwA = NAN, compareAwB = NAN;
int compareCatA = 0, compareCatB = 0;
unsigned long compareDurationA = 0, compareDurationB = 0; // v14: เวลาที่ใช้วัดแต่ละตัวอย่างในโหมดเปรียบเทียบ
bool compareSaved = false;    // v12: ผู้ใช้เลือก Save both ตอนตอบป็อปอัปของตัวอย่าง B หรือไม่ (ใช้โชว์ในหน้าสรุปผล)

// ---------------- v12: ป็อปอัปบนจอบอร์ดเมื่อกราฟนิ่ง (ยังวัดต่อจนกว่าผู้ใช้จะเลือก) ----------------
bool savePromptActive = false;    // true = ป็อปอัปกำลังแสดงอยู่ทับกราฟ (ค้างไว้จนกว่าจะกดเลือกหรือกดออก)
int promptSel = 0;                // 0 = ตัวเลือกบน (Save / Yes,measure B / Save both), 1 = ตัวเลือกล่าง
float promptAw = 0;               // ค่า aw ล่าสุด (เฉลี่ยจากหน้าต่างนิ่ง) ณ รอบล่าสุด — คือค่าที่จะถูกบันทึกถ้ากด Save
unsigned long promptDurSec = 0;   // เวลาที่วัดจริง (วินาที) นับตั้งแต่เริ่มวัด ไม่รวมช่วงฮีต/รอเย็นก่อนวัด

// ---------------- v12: สถานะของขั้นตอนฮีต/รอเย็นเซนเซอร์ SHT ก่อน/หลังวัด ----------------
CondNext condNext = COND_NEXT_MENU_AW;   // ทำอะไรต่อเมื่อจบขั้นตอนนี้
bool condIsPre = true;                   // true = ก่อนวัด, false = หลังวัด (ใช้เลือกข้อความบนจอ + ปุ่มข้าม)
int condReturnSel = 0;                   // ตำแหน่งเมนู AW ที่จะกลับไปเมื่อยกเลิก/จบ (0=Start 1=Predict 2=Compare)
int condStep = 0;                        // 0 = กำลังฮีต, 1 = กำลังรอเย็น
unsigned long condStepStartMs = 0;       // เวลาเริ่มขั้นตอนย่อยปัจจุบัน
unsigned long condHeatMs = 0, condCoolMs = 0;
unsigned long condLastUiMs = 0;
// ---------------- Phase 3 (calibration roadmap): ปรับเวลา "รอเย็น" (precool) อัตโนมัติจากค่าจริง ----------------
// แนวคิด: condCoolMs (ค่าคงที่ SHT_PRECOOL_MS) ยังเป็น "เพดานสูงสุด" เท่าเดิมเป๊ะ ๆ - ไม่มีทางรอนานกว่านี้
// แต่ถ้าตัวชิปเย็นลงจนใกล้เคียงตัวอย่างจริง (วัดจาก gradientDeltaC() ที่ใช้อยู่แล้วในสูตรชดเชย Magnus) ก่อนครบเวลา
// เต็ม จะข้ามไปขั้นถัดไปได้เลย ไม่ต้องรอเปล่า ๆ จึงปลอดภัย 100% (แย่สุดคือรอเท่าของเดิม ไม่มีทางรอน้อยกว่าที่ควร
// เพราะต้องนิ่งต่อเนื่องกันหลายครั้งก่อน ไม่ใช่แค่ค่าแวบเดียว)
const unsigned long COND_PRECOOL_MIN_MS = SHT_PRECOOL_MS; // ห้าม early-exit: ต้องรอเต็มเวลาเพื่อไล่ thermal ripple/ฟิล์มความชื้น   // ไม่ยอมให้ข้ามก่อนเวลานี้ไม่ว่ากรณีใด กันฮีตเตอร์ตกค้าง/ค่ากระเพื่อมทันทีหลังปิดฮีต
const int COND_PRECOOL_SETTLE_STREAK = 3;           // ต้องนิ่งในเกณฑ์ติดต่อกันกี่ครั้ง (เช็คทุก 250ms พร้อม UI) ก่อนเชื่อว่านิ่งจริง
int condPrecoolSettledStreak = 0;                   // นับต่อเนื่อง reset ทุกครั้งที่เข้าขั้นรอเย็นใหม่
unsigned long condLastTempRefreshMs = 0;            // throttle การเรียก refreshShtTempOnly() ระหว่างรอเย็น (ทุก ~1 วิ พอ)
AppState state = ST_BOOT_WARMUP;
int selIndex = 0;
AppState prevDrawnState = (AppState)-1;
int prevDrawnSel = -1;

const char* mainItems[] = { "1.Measure AW", "2.Recording", "3.WiFi Info", "4.System Health" };
const IconType mainIcons[] = { ICON_MEASURE, ICON_RECORD, ICON_WIFI, ICON_HEALTH };
const char* awItems[] = { "1.1 Start", "1.2 Predict", "1.3 Compare", "1.4 Cancel" };
const IconType awIcons[] = { ICON_MEASURE, ICON_PREDICT, ICON_COMPARE, ICON_CANCEL };

#define MAX_RECORDINGS 5
#define REC_SNAPSHOT_PTS 24

struct RecEntry {
  uint32_t id;
  float aw;
  uint8_t cat;
  uint32_t durationSec; // v14: เวลาที่ใช้วัดจริง (วินาที) — 0 = ไม่มีข้อมูล (บันทึกไว้ก่อน v14)
  uint32_t ts;          // v-pro: เวลา epoch ตอนบันทึก (0 = unsynced/บันทึกไว้ก่อน v-pro ไม่มีข้อมูล)
  char op[9];           // v-pro: ชื่อย่อผู้ปฏิบัติงานตอนบันทึก (audit trail) — ว่าง = ไม่ได้ตั้งชื่อไว้
};
int recCount = 0;
RecEntry recList[MAX_RECORDINGS];
int recSlotOf[MAX_RECORDINGS];
float viewSnap[REC_SNAPSHOT_PTS];

int recItemIndex = -1;
char recItemTitle[24];
const char* recItemItems[] = { "View", "Delete", "Cancel" };
const IconType recItemIcons[] = { ICON_RECORD, ICON_DELETE, ICON_CANCEL };

void downsampleGraph(float* out, int n) {
  if (numPoints <= 0) {
    for (int i = 0; i < n; i++) out[i] = 0;
    return;
  }
  for (int i = 0; i < n; i++) {
    int srcIdx = (n <= 1) ? 0 : (int)((long)i * (numPoints - 1) / (n - 1));
    out[i] = values[srcIdx];
  }
}

void saveRecording(float finalAw, int catIdx, unsigned long durationSec) {
  prefs.begin("awrec", false);
  int count = prefs.getUChar("count", 0);
  int next = prefs.getUChar("next", 0);
  uint32_t newId = prefs.getUInt("lastid", 0) + 1;
  float snap[REC_SNAPSHOT_PTS];
  downsampleGraph(snap, REC_SNAPSHOT_PTS);
  String si = String(next);
  prefs.putUInt(("id" + si).c_str(), newId);
  prefs.putFloat(("aw" + si).c_str(), finalAw);
  prefs.putUChar(("cat" + si).c_str(), (uint8_t)catIdx);
  prefs.putUInt(("dur" + si).c_str(), (uint32_t)durationSec);
  prefs.putUInt(("ts" + si).c_str(), (uint32_t)nowEpoch());   // v-pro: audit trail - เวลาบันทึกจริง (0 = ยังไม่ซิงก์นาฬิกา)
  prefs.putString(("op" + si).c_str(), String(operatorTag));  // v-pro: audit trail - ผู้ปฏิบัติงาน ณ ขณะบันทึก
  prefs.putBytes(("pts" + si).c_str(), snap, sizeof(snap));
  next = (next + 1) % MAX_RECORDINGS;
  if (count < MAX_RECORDINGS) count++;
  prefs.putUChar("count", count);
  prefs.putUChar("next", next);
  prefs.putUInt("lastid", newId);
  prefs.end();
  lcd.setCursor(0, 1);
  lcd.print("Saved!          ");
  tft.fillRect(0, screenH - 14, screenW, 14, COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_OK, COL_BG);
  tft.drawString("Saved", screenW / 2, screenH - 13, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
  delay(500);
}

void loadRecordingIndex() {
  prefs.begin("awrec", true);
  recCount = prefs.getUChar("count", 0);
  int next = prefs.getUChar("next", 0);
  for (int k = 0; k < recCount; k++) {
    int slot = (next - 1 - k + MAX_RECORDINGS * 2) % MAX_RECORDINGS;
    String si = String(slot);
    recList[k].id = prefs.getUInt(("id" + si).c_str(), 0);
    recList[k].aw = prefs.getFloat(("aw" + si).c_str(), 0);
    recList[k].cat = prefs.getUChar(("cat" + si).c_str(), 0);
    recList[k].durationSec = prefs.getUInt(("dur" + si).c_str(), 0); // 0 = บันทึกไว้ก่อน v14 ไม่มีข้อมูลเวลา
    recList[k].ts = prefs.getUInt(("ts" + si).c_str(), 0);           // 0 = บันทึกไว้ก่อน v-pro หรือยังไม่ซิงก์นาฬิกาตอนนั้น
    prefs.getString(("op" + si).c_str(), "").toCharArray(recList[k].op, sizeof(recList[k].op));
    recSlotOf[k] = slot;
  }
  prefs.end();
}

void loadRecordingSnapshot(int slot) {
  prefs.begin("awrec", true);
  String si = String(slot);
  prefs.getBytes(("pts" + si).c_str(), viewSnap, sizeof(viewSnap));
  prefs.end();
}

void deleteRecording(int viewIndex) {
  int oldCount = recCount;
  uint32_t ids[MAX_RECORDINGS];
  float aws[MAX_RECORDINGS];
  uint8_t cats[MAX_RECORDINGS];
  uint32_t durs[MAX_RECORDINGS];
  uint32_t tss[MAX_RECORDINGS];
  static char ops[MAX_RECORDINGS][9];
  static float pts[MAX_RECORDINGS][REC_SNAPSHOT_PTS];
  int idx = 0;
  prefs.begin("awrec", true);
  uint32_t lastId = prefs.getUInt("lastid", 0);
  for (int k = oldCount - 1; k >= 0; k--) {
    if (k == viewIndex) continue;
    String si = String(recSlotOf[k]);
    ids[idx] = recList[k].id;
    aws[idx] = recList[k].aw;
    cats[idx] = recList[k].cat;
    durs[idx] = recList[k].durationSec;
    tss[idx] = recList[k].ts;
    strncpy(ops[idx], recList[k].op, sizeof(ops[idx]));
    prefs.getBytes(("pts" + si).c_str(), pts[idx], sizeof(pts[idx]));
    idx++;
  }
  prefs.end();
  prefs.begin("awrec", false);
  prefs.clear();
  for (int i = 0; i < idx; i++) {
    String si = String(i);
    prefs.putUInt(("id" + si).c_str(), ids[i]);
    prefs.putFloat(("aw" + si).c_str(), aws[i]);
    prefs.putUChar(("cat" + si).c_str(), cats[i]);
    prefs.putUInt(("dur" + si).c_str(), durs[i]);
    prefs.putUInt(("ts" + si).c_str(), tss[i]);
    prefs.putString(("op" + si).c_str(), String(ops[i]));
    prefs.putBytes(("pts" + si).c_str(), pts[i], sizeof(pts[i]));
  }
  prefs.putUChar("count", (uint8_t)idx);
  prefs.putUChar("next", (uint8_t)(idx % MAX_RECORDINGS));
  prefs.putUInt("lastid", lastId);
  prefs.end();
  loadRecordingIndex();
}

void drawRecordListScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RECORDING");
  lcd.setCursor(0, 1);
  if (recCount == 0) {
    lcd.print("(empty)");
  } else if (selIndex == recCount) {
    lcd.print(">Back");
  } else {
    char line[17];
    snprintf(line, sizeof(line), ">#%lu aw%.3f", (unsigned long)recList[selIndex].id, recList[selIndex].aw);
    lcd.print(line);
  }
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Recording", screenW / 2, 4, 2);
  tft.setTextDatum(TL_DATUM);
  if (recCount == 0) {
    tft.setTextDatum(MC_DATUM);
    tft.drawString("No saved graphs yet", screenW / 2, screenH / 2, 2);
    tft.setTextDatum(TL_DATUM);
    return;
  }
  int y = 30;
  for (int i = 0; i < recCount; i++) {
    bool sel = (i == selIndex);
    tft.setTextColor(sel ? COL_TOUCH : COL_TEXT, COL_BG);
    char durbuf[12];
    formatDurationShort(recList[i].durationSec, durbuf, sizeof(durbuf));
    char line[32];
    snprintf(line, sizeof(line), "%s#%lu aw:%.3f %s", sel ? ">" : " ",
             (unsigned long)recList[i].id, recList[i].aw, durbuf);
    tft.drawString(line, 6, y, 1);
    y += 12;
  }
  bool selBack = (selIndex == recCount);
  tft.setTextColor(selBack ? COL_TOUCH : COL_TEXT, COL_BG);
  tft.drawString(selBack ? ">Back" : " Back", 6, y, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
}

void drawRecordViewScreen() {
  int idx = selIndex;
  tft.fillScreen(COL_BG);
  drawGraphFrame();
  char title[24];
  snprintf(title, sizeof(title), "#%lu  aw:%.3f", (unsigned long)recList[idx].id, recList[idx].aw);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(title, screenW / 2, 4, 2);
  tft.setTextDatum(TL_DATUM);
  for (int i = 0; i < REC_SNAPSHOT_PTS - 1; i++) {
    int x1 = graphX + i * (graphW - 1) / (REC_SNAPSHOT_PTS - 1);
    int x2 = graphX + (i + 1) * (graphW - 1) / (REC_SNAPSHOT_PTS - 1);
    tft.drawLine(x1, valueToY(viewSnap[i]), x2, valueToY(viewSnap[i + 1]), COL_OK);
  }
  drawFinalDurationTFT(recList[idx].durationSec);
  // v-pro: audit trail - โชว์เวลาที่บันทึก + ผู้ปฏิบัติงาน (ถ้ามี) มุมล่างซ้ายของกราฟ
  char tsbuf[20];
  formatEpoch((time_t)recList[idx].ts, tsbuf, sizeof(tsbuf));
  String auditLine = String(tsbuf);
  if (strlen(recList[idx].op) > 0) auditLine += " " + String(recList[idx].op);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(auditLine, 4, screenH - 10, 1);
  lcd.clear();
  lcd.setCursor(0, 0);
  char l1[17];
  snprintf(l1, sizeof(l1), "aw:%.3f #%lu", recList[idx].aw, (unsigned long)recList[idx].id);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  char l2[17];
  formatDurationShort(recList[idx].durationSec, l2, sizeof(l2));
  String lbl = String(l2);
  while (lbl.length() < 16) lbl += ' ';
  lcd.print(lbl.substring(0, 16));
}

// ============================================================================
//  v12: ป็อปอัปบนจอบอร์ดเมื่อกราฟนิ่ง + โปรแกรมฮีต/รอเย็นเซนเซอร์ SHT ก่อน/หลังวัด
// ============================================================================

// ค่าเฉลี่ยของค่าดิบในหน้าต่างนิ่งล่าสุด (30 วินาทีตามจำนวนตัวอย่าง) แปลงเป็น aw — คือค่าที่ใช้บันทึก
float stabWindowMeanAw() {
  // v16: เฉลี่ยเฉพาะบล็อกในหน้าต่างนิ่ง 60 วิ (ไม่ใช่ทั้งบัฟเฟอร์ดิบ) — ตรงกับค่าที่ล็อกใน updateStabilityWindow()
  int n = (stabBlkN < STAB_WIN_BLKS) ? stabBlkN : STAB_WIN_BLKS;
  if (n <= 0) return currentAw;
  float sum = 0;
  for (int i = stabBlkN - n; i < stabBlkN; i++) sum += stabBlkRaw[i];
  return applyCal(sum / n, currentTempC);
}

// อัปเดตค่าที่ป็อปอัปโชว์/จะบันทึก ให้เป็นค่าล่าสุดเสมอ (เรียกทุกรอบวัดตราบที่ป็อปอัปยังค้างอยู่)
void refreshPromptValues() {
  promptAw = stabWindowMeanAw();
  promptDurSec = (millis() - measureStartMs) / 1000UL;
  // v-pro: เตือน "Sensor noise?" ถ้าค่าดิบในหน้าต่างกระเพื่อมกว้างผิดปกติ แม้กราฟที่กรองแล้วจะดูนิ่ง
  float mn, mx;
  getStabRange(mn, mx);
  lastMeasureNoiseWarning = (mx - mn) > RAW_NOISE_WARN_RANGE;
}

// ข้อความป็อปอัปตามโหมดปัจจุบัน — วัดปกติ / เปรียบเทียบ A / เปรียบเทียบ B
const PromptText PROMPT_SINGLE = { "",  "Save this result?", "Save",           "Don't save", ">Save Don't save", " Save>Don't save" };
const PromptText PROMPT_CMP_A  = { "A", "Measure B next?",   "Yes, measure B", "No, cancel",  ">Meas.B  Cancel ", " Meas.B >Cancel " };
const PromptText PROMPT_CMP_B  = { "B", "Save A and B?",     "Save both",      "Don't save", ">Save both No   ", " Save both>No   " };
const PromptText& currentPromptText() {
  if (state == ST_COMPARE_MEASURE) return (compareStage == 0) ? PROMPT_CMP_A : PROMPT_CMP_B;
  return PROMPT_SINGLE;
}

// วาดกล่องป็อปอัปทับกลางกราฟ — ต้องเรียกหลัง drawGraph() ทุกรอบ เพราะ drawGraph() ล้างพื้นที่กราฟใหม่ทุกรอบ
// (กราฟด้านหลังยังวิ่งต่อเนื่องตามปกติ ป็อปอัปแค่ถูกวาดซ้อนทับ)
void drawPromptPopupTFT() {
  const PromptText& pt = currentPromptText();
  bool big = (graphH >= 100);            // จอใหญ่ใช้ฟอนต์ 2 (16px) จอเล็กใช้ฟอนต์ 1 (8px)
  int font = big ? 2 : 1;
  int lh = big ? 18 : 12;
  int lines = lastMeasureNoiseWarning ? 5 : 4;
  int bw = graphW - 10; if (bw > 180) bw = 180;
  int bh = lines * lh + 8; if (bh > graphH - 2) bh = graphH - 2;
  int bx = graphX + (graphW - bw) / 2;
  int by = graphY + (graphH - bh) / 2;

  tft.fillRect(bx, by, bw, bh, COL_BG);
  tft.drawRect(bx, by, bw, bh, COL_OK);
  tft.drawRect(bx + 1, by + 1, bw - 2, bh - 2, COL_OK);
  tft.setTextDatum(TL_DATUM);
  int tx = bx + 7, ty = by + 5;

  char title[28];
  if (pt.tag[0]) snprintf(title, sizeof(title), "%s STABLE aw:%.3f", pt.tag, promptAw);
  else snprintf(title, sizeof(title), "STABLE aw:%.3f", promptAw);
  tft.setTextColor(COL_OK, COL_BG);
  tft.drawString(title, tx, ty, font);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(pt.question, tx, ty + lh, font);
  for (int i = 0; i < 2; i++) {
    tft.setTextColor(i == promptSel ? COL_TOUCH : COL_TEXT, COL_BG);
    tft.drawString(String(i == promptSel ? "> " : "  ") + (i == 0 ? pt.opt0 : pt.opt1), tx, ty + (2 + i) * lh, font);
  }
  if (lastMeasureNoiseWarning) {
    tft.setTextColor(COL_WARN, COL_BG);
    tft.drawString("Sensor noise?", tx, ty + 4 * lh, font);
  }
  tft.setTextColor(COL_TEXT, COL_BG);
}

// จอ LCD 16x2 ระหว่างป็อปอัป: บรรทัดบนโชว์ค่า aw ล่าสุด บรรทัดล่างโชว์ตัวเลือก (เครื่องหมาย > ชี้ตัวเลือกที่เลือกอยู่)
void drawPromptLCD() {
  const PromptText& pt = currentPromptText();
  char l1[17];
  if (pt.tag[0]) snprintf(l1, sizeof(l1), "[%s]%.3f STABLE%s", pt.tag, promptAw, lastMeasureNoiseWarning ? "!" : "");
  else snprintf(l1, sizeof(l1), "aw:%.3f STAB%s", promptAw, lastMeasureNoiseWarning ? "!" : "");
  String s1 = String(l1);
  while (s1.length() < 16) s1 += ' ';
  lcd.setCursor(0, 0);
  lcd.print(s1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(promptSel == 0 ? pt.lcd0 : pt.lcd1);
}

// ผู้ใช้กดยืนยัน (ค้าง 1 ปุ่ม) ที่ป็อปอัปของโหมดวัดปกติ
void handleMeasurePromptChoice() {
  refreshPromptValues();  // ใช้ค่า ณ วินาทีที่กด ไม่ใช่ค่าตอนป็อปอัปเพิ่งเด้ง
  if (promptSel == 0) saveRecording(promptAw, getFoodCategoryIndex(promptAw), promptDurSec);
  savePromptActive = false;
  startSensorConditioning(false, COND_NEXT_MENU_AW, 0);  // หลังวัดฮีตไล่ไอน้ำ แล้วกลับเมนู
}

// ผู้ใช้กดยืนยันที่ป็อปอัปของโหมดเปรียบเทียบ (ตัวอย่าง A หรือ B)
void handleComparePromptChoice() {
  refreshPromptValues();
  savePromptActive = false;
  if (compareStage == 0) {
    if (promptSel == 0) {
      // Yes: เก็บค่า A แล้วฮีต/รอเย็นเซนเซอร์ก่อนเริ่มวัด B (เป็นช่วงที่เปลี่ยนตัวอย่างไปด้วยได้)
      compareAwA = promptAw;
      compareCatA = getFoodCategoryIndex(compareAwA);
      compareDurationA = promptDurSec;
      startSensorConditioning(true, COND_NEXT_COMPARE_B, 2);
    } else {
      // No: ยกเลิกโหมดเปรียบเทียบทั้งหมด ไม่บันทึกอะไร
      startSensorConditioning(false, COND_NEXT_MENU_AW, 2);
    }
  } else {
    compareAwB = promptAw;
    compareCatB = getFoodCategoryIndex(compareAwB);
    compareDurationB = promptDurSec;
    compareSaved = (promptSel == 0);
    if (compareSaved) {
      saveRecording(compareAwA, compareCatA, compareDurationA);
      saveRecording(compareAwB, compareCatB, compareDurationB);
    }
    state = ST_COMPARE_RESULT;          // โชว์หน้าสรุปผล A vs B (บอกสถานะบันทึก/ไม่บันทึกไว้ที่หน้านั้น)
    prevDrawnState = (AppState)-1;
  }
}

// ---------- โปรแกรมฮีต/รอเย็นเซนเซอร์ SHT (non-blocking: loop() ยังวิ่ง เว็บ/watchdog/PID ทำงานปกติ) ----------
void drawSensorCondStatic() {
  bool heating = (condStep == 0);
  bool bigFont = (screenW >= 240);
  // v22: เปลี่ยนจาก ฮีต=ม่วง/รอเย็น=น้ำเงิน เป็น ฮีต=น้ำเงิน/รอเย็น=ฟ้าอมเขียว(cyan) — สงวนม่วงไว้ให้ความหมาย "fault" เท่านั้น
  // (ไม่งั้นระหว่างฮีตเซนเซอร์ปกติจะติดสีเดียวกับตอนเซนเซอร์เสีย ทำให้สับสนว่าเครื่องกำลังฮีตหรือกำลังพัง)
  setLED(false, !heating, true);
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_MAGENTA, COL_BG);
  tft.drawString(condIsPre ? "SHT HEATER - BEFORE MEASURE" : "SHT HEATER - AFTER MEASURE", screenW / 2, 8, 2);
  tft.setTextColor(heating ? TFT_ORANGE : TFT_CYAN, COL_BG);
  tft.drawString(heating ? "HEATING" : "COOLING DOWN", screenW / 2, screenH / 2 - 34, bigFont ? 4 : 2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(condIsPre ? "Please wait - measurement starts after" : "Cleaning sensor - back to menu after",
                 screenW / 2, screenH - 34, 1);
  tft.setTextColor(COL_AXIS, COL_BG);
  tft.drawString(condIsPre ? "BOTH buttons = Cancel" : "HOLD = Skip   BOTH = Exit", screenW / 2, screenH - 16, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
  condLastUiMs = 0;  // บังคับให้วาดตัวเลขนับถอยหลังทันทีรอบถัดไป
}

void drawSensorCondDynamic(unsigned long elapsedMs, unsigned long totalMs) {
  unsigned long leftSec = (elapsedMs >= totalMs) ? 0 : (totalMs - elapsedMs + 999UL) / 1000UL;
  bool bigFont = (screenW >= 240);
  char buf[12];
  snprintf(buf, sizeof(buf), "%02u:%02u", (unsigned)(leftSec / 60UL), (unsigned)(leftSec % 60UL));
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.fillRect(0, screenH / 2 - 2, screenW, bigFont ? 30 : 20, COL_BG);
  tft.drawString(buf, screenW / 2, screenH / 2 - 2, bigFont ? 4 : 2);

  int bx = 16, bw = screenW - 32, by = screenH / 2 + 34, bh = 12;
  int fillW = (totalMs > 0) ? (int)((unsigned long)(bw - 2) * (elapsedMs > totalMs ? totalMs : elapsedMs) / totalMs) : (bw - 2);
  tft.drawRect(bx, by, bw, bh, COL_AXIS);
  tft.fillRect(bx + 1, by + 1, fillW, bh - 2, condStep == 0 ? TFT_ORANGE : TFT_CYAN);
  tft.fillRect(bx + 1 + fillW, by + 1, (bw - 2) - fillW, bh - 2, COL_BG);
  tft.setTextDatum(TL_DATUM);

  char l1[17], l2[17];
  snprintf(l1, sizeof(l1), "%-16s", condIsPre ? "SHT heat: BEFORE" : "SHT heat: AFTER");
  snprintf(l2, sizeof(l2), "%-9s %s", condStep == 0 ? "Heating" : "Cooling", buf);
  lcd.setCursor(0, 0);
  lcd.print(String(l1).substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print((String(l2) + "                ").substring(0, 16));
}

void startSensorConditioning(bool isPre, CondNext next, int returnSel) {
  condIsPre = isPre;
  condNext = next;
  condReturnSel = returnSel;
  condHeatMs = isPre ? SHT_PREHEAT_MS : SHT_POSTHEAT_MS;
  condCoolMs = isPre ? SHT_PRECOOL_MS : SHT_POSTCOOL_MS;
  savePromptActive = false;
  iconVisible = false;
  heaterAutoActive = false;  // ถ้า auto heater กำลังฮีตอยู่ ให้ขั้นตอนนี้รับช่วงคุมฮีตเตอร์ต่อ (กัน auto ปิดฮีตเตอร์กลางคัน)
  highRhStartMs = 0;
  state = ST_SENSOR_COND;
  prevDrawnState = state;    // ไม่ต้องให้ลูปหลักวาดเมนูทับ
  condStep = (condHeatMs > 0) ? 0 : 1;
  condStepStartMs = millis();
  condPrecoolSettledStreak = 0;  // Phase 3: เริ่มนับใหม่ทุกครั้งที่เข้ารอบคาลิเบรตความร้อนใหม่
  if (condStep == 0) setSensorHeater(true);
  else if (condCoolMs == 0) { finishSensorConditioning(); return; }  // ตั้งเวลาเป็น 0 ทั้งคู่ = ข้ามขั้นตอนนี้ไปเลย
  lcd.clear();
  drawSensorCondStatic();
}

void finishSensorConditioning() {
  setSensorHeater(false);  // กันพลาด: ทางออกทุกทางของขั้นตอนนี้ต้องปิดฮีตเตอร์เสมอ
  heaterLastAutoFireMs = millis();  // นับเป็นการฮีตแล้ว ให้ auto heater รอบบำรุงรักษานับเวลาใหม่จากตรงนี้
  switch (condNext) {
    case COND_NEXT_MEASURE:   beginMeasureAW(); break;
    case COND_NEXT_PREDICT:   beginPredictAW(); break;
    case COND_NEXT_COMPARE_A: beginCompareA(); break;
    case COND_NEXT_COMPARE_B: beginCompareB(); break;
    case COND_NEXT_MENU_AW:
    default:
      setLED(false, false, false);
      state = ST_MENU_AW;
      selIndex = condReturnSel;
      prevDrawnState = (AppState)-1;  // ให้ลูปหลักวาดเมนู AW ใหม่ (drawMenuScreen ล้างทั้ง TFT และ LCD เอง)
      break;
  }
}

// ผู้ใช้กดออก (กด 2 ปุ่มพร้อมกัน) ระหว่างฮีต/รอเย็น — ปิดฮีตเตอร์แล้วกลับเมนู AW ไม่เริ่มวัด
void cancelSensorConditioning() {
  setSensorHeater(false);
  heaterLastAutoFireMs = millis();
  setLED(false, false, false);
  state = ST_MENU_AW;
  selIndex = condReturnSel;
  prevDrawnState = (AppState)-1;
}

void runSensorCondTick(const ButtonEvents& e) {
  if (e.exit) { cancelSensorConditioning(); return; }
  // หลังวัดอนุญาตให้ข้ามได้ (ตัวอย่างวัดเสร็จแล้ว ไม่กระทบผลวัด) แต่ก่อนวัดห้ามข้าม เพื่อไม่ให้ใครเริ่มวัดด้วยเซนเซอร์ที่ยังร้อนอยู่
  if (!condIsPre && e.select) { finishSensorConditioning(); return; }

  unsigned long now = millis();
  unsigned long total = (condStep == 0) ? condHeatMs : condCoolMs;
  unsigned long elapsed = now - condStepStartMs;
  // SHT45 heater เป็น pulse 1 วินาที: ยิงซ้ำทุก 900 ms ตลอดช่วง heating เพื่อให้เวลาฮีตใน UI ตรงกับฮาร์ดแวร์
  if (condStep == 0 && humSensorType == HUMSENS_SHT && sensorHeaterOn && now - lastShtHeaterPulseMs >= 900UL) {
    sht.heater(true); lastShtHeaterPulseMs = now;
  }

  // Phase 3: ระหว่างขั้น "รอเย็น" ต้องอัปเดตอุณหภูมิชิป SHT สด ๆ เป็นระยะ (ไม่งั้น gradientDeltaC() จะเห็นค่าค้าง
  // เกิน 5 วิ แล้วคืน NaN เสมอ ทำให้ early-exit ด้านล่างไม่มีทางทำงานได้จริง) - ทำเฉพาะช่วงนี้ ไม่กระทบ %RH/currentAw
  if (condStep == 1 && now - condLastTempRefreshMs >= 1000UL) {
    condLastTempRefreshMs = now;
    refreshShtTempOnly();
  }

  // Phase 3: ระหว่างขั้น "รอเย็น" เช็คว่าตัวชิปเย็นลงจนใกล้เคียงตัวอย่างจริงหรือยัง (ก่อนครบเวลาเต็ม)
  // ปลอดภัยเพราะ total (condCoolMs) ยังเป็นเพดานสูงสุดเท่าเดิมเป๊ะ ๆ - จะเสร็จช้ากว่านี้ไม่ได้ มีแต่เสร็จเร็วกว่าได้
  // เมื่อมีหลักฐานจริงว่านิ่งแล้วเท่านั้น (ดูคำอธิบายที่จุดประกาศตัวแปร condPrecoolSettledStreak ด้านบนไฟล์)
  if (condStep == 1 && elapsed >= COND_PRECOOL_MIN_MS && elapsed < total) {
    float g = gradientDeltaC();
    if (!isnan(g) && fabs(g) <= gradientDeadbandC()) {
      if (++condPrecoolSettledStreak >= COND_PRECOOL_SETTLE_STREAK) {
        Serial.printf("[COND] precool settled early after %lums (cap was %lums)\n", elapsed, total);
        finishSensorConditioning();
        return;
      }
    } else {
      condPrecoolSettledStreak = 0; // ยังไม่นิ่งจริง หรืออ่านค่าไม่ได้รอบนี้ - เริ่มนับใหม่ ไม่สะสมข้ามรอบที่ไม่นิ่ง
    }
  }

  if (elapsed >= total) {
    if (condStep == 0) {
      setSensorHeater(false);  // ครบเวลาฮีต ปิดทันที
      heaterLastAutoFireMs = now;
      if (condCoolMs == 0) { finishSensorConditioning(); return; }
      condStep = 1;
      condStepStartMs = now;
      condPrecoolSettledStreak = 0;  // Phase 3: เริ่มนับความนิ่งใหม่ตอนเข้าขั้นรอเย็น
      drawSensorCondStatic();
      return;
    }
    finishSensorConditioning();
    return;
  }
  if (now - condLastUiMs >= 250) {
    condLastUiMs = now;
    drawSensorCondDynamic(elapsed, total);
  }
}

// ชื่อเฟสของเครื่องที่ส่งให้เว็บทาง /data — เว็บใช้ตัดสินว่าช่วงนี้ห้ามบันทึกค่าลงกราฟ (เซนเซอร์กำลังร้อน/กำลังเย็นลง)
const char* sensorPhaseName() {
  if (state == ST_SENSOR_COND) {
    if (condIsPre) return (condStep == 0) ? "preheat" : "precool";
    return (condStep == 0) ? "postheat" : "postcool";
  }
  if (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE) return "measuring";
  return "idle";
}

// ---------- Web Server Callbacks ----------
// v-web-ctrl: เปิดล็อกอินเว็บด้วย HTTP Basic Auth (ป๊อปอัปมาตรฐานของเบราว์เซอร์) มี 2 บัญชี:
//   person / 12345678  -> ผู้ใช้ทั่วไป (ROLE_USER)   เห็นกราฟค่าที่ใช้งานจริง สั่งเริ่มวัดจากเว็บได้
//   admin  / golf4981  -> แอดมิน      (ROLE_ADMIN)  เห็นทั้งกราฟจริงและกราฟคาลิเบรตซ้อนกัน + มีโหมดคาลิเบตอัตโนมัติ 5 รอบ
// แก้ชื่อผู้ใช้/รหัสผ่านได้ตรงนี้ที่เดียว ทุก handler เรียก checkAuth() นี้อยู่แล้วเหมือนเดิม
const char* AUTH_USER_PERSON = "person";
const char* AUTH_PASS_PERSON = "12345678";
const char* AUTH_USER_ADMIN  = "admin";
const char* AUTH_PASS_ADMIN  = "golf4981";
bool checkAuth() {
  if (server.authenticate(AUTH_USER_ADMIN, AUTH_PASS_ADMIN)) { currentRole = ROLE_ADMIN; return true; }
  if (server.authenticate(AUTH_USER_PERSON, AUTH_PASS_PERSON)) { currentRole = ROLE_USER; return true; }
  currentRole = ROLE_NONE;
  server.requestAuthentication(BASIC_AUTH, "AW Meter Dashboard");
  return false;
}

// v-web-ctrl: /whoami — ให้หน้าเว็บถามว่าล็อกอินเป็นใคร เพื่อโชว์/ซ่อนแผงเฉพาะแอดมิน (กราฟคู่ + โหมดคาลิเบตอัตโนมัติ)
void handleWhoAmI() {
  if (!checkAuth()) return;
  const char* roleStr = (currentRole == ROLE_ADMIN) ? "admin" : (currentRole == ROLE_USER) ? "person" : "none";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", String("{\"role\":\"") + roleStr + "\"}");
}

// v-web-ctrl: GET /cmd/measure — ปุ่ม "เริ่มบันทึกกราฟ" บนเว็บกดแล้ว = สั่งบอร์ดเริ่มวัดจริงทันที (เหมือนกดปุ่มที่ตัวเครื่อง)
// ต้องล็อกอินก่อน (checkAuth) และเครื่องต้องว่าง/ไม่ติดโหมดคาลิเบตอัตโนมัติอยู่
void handleCmdMeasure() {
  if (!checkAuth()) return;
  if (autoCalPhase != ACAL_IDLE && autoCalPhase != ACAL_DONE) {
    server.send(409, "text/plain", "Device is running admin auto-calibration mode");
    return;
  }
  if (state != ST_MENU_MAIN && state != ST_MENU_AW) {
    server.send(409, "text/plain", "Device busy (not idle at menu)");
    return;
  }
  const char* reason = "";
  if (!canStartMeasurement(&reason)) {
    server.send(409, "text/plain", reason);
    return;
  }
  enterMeasureAW();  // ฟังก์ชันเดิม: เข้าสู่ขั้นฮีต/รอเย็นเซนเซอร์ก่อน แล้วค่อยเริ่มวัดจริง (เหมือนกดปุ่มที่ตัวเครื่อง 1.1 Start ทุกประการ)
  server.send(200, "text/plain", "OK");
}

// v-web-ctrl: GET /cmd/cancel — ยกเลิกการวัดที่กำลังทำอยู่จากเว็บ กลับไปเมนู AW บนตัวเครื่อง
void handleCmdCancel() {
  if (!checkAuth()) return;
  if (state == ST_MEASURE_AW || state == ST_SENSOR_COND || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE) {
    state = ST_MENU_AW;
    selIndex = 0;
    savePromptActive = false;
    prevDrawnState = (AppState)-1;
  }
  server.send(200, "text/plain", "OK");
}

// v-web-ctrl: GET /admin/calmode/start — แอดมินเท่านั้น เริ่มโหมดคาลิเบตอัตโนมัติ 10 รอบ (วัด/ทดสอบ อุณหภูมิ 25/25/25/20/19 °C จบรอบเมื่อค่านิ่ง)
void handleAdminCalStart() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  if (autoCalPhase == ACAL_COOLING || autoCalPhase == ACAL_MEASURING) {
    server.send(409, "text/plain", "Auto-calibration already running");
    return;
  }
  if (state != ST_MENU_MAIN && state != ST_MENU_AW) {
    server.send(409, "text/plain", "Device busy (not idle at menu)");
    return;
  }
  const char* reason = "";
  if (!canStartMeasurement(&reason)) {
    server.send(409, "text/plain", reason);
    return;
  }
  // v-acal-graph: ขั้นตอนแรกบังคับกรอกค่าเป้าหมาย (aw อ้างอิงของสารละลายมาตรฐาน) มาด้วยเสมอ ไม่งั้นจะคาลิเบตไม่ถูกต้อง
  if (!server.hasArg("ref")) {
    server.send(400, "text/plain", "Missing ref (target aw) — must be entered in step 1 before starting");
    return;
  }
  float refAw = server.arg("ref").toFloat();
  if (isnan(refAw) || refAw <= 0.0f || refAw > 1.05f) {
    server.send(400, "text/plain", "Invalid ref (target aw) — must be a number between 0 and 1");
    return;
  }
  autoCalRefAw = refAw;
  startAutoCalMode();
  server.send(200, "text/plain", "OK");
}

// v-web-ctrl: GET /admin/calmode/cancel — แอดมินเท่านั้น ยกเลิกโหมดคาลิเบตอัตโนมัติกลางคัน
void handleAdminCalCancel() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  cancelAutoCalMode();
  server.send(200, "text/plain", "OK");
}

// v27: สร้างพื้นผิว 2 มิติจากผลคาลิเบตอัตโนมัติ (ช่อง 25 / 20 / 19 °C) — แอดมินสั่งเองเท่านั้น
// หลักการ: ตัวอย่างเดียวกันควรได้ aw เท่ากันทุกอุณหภูมิ => ตัวคูณที่ช่องอุณหภูมิ T = aw_ที่_25°C_ตามสมการปัจจุบัน / aw_ที่_T_ตามสมการปัจจุบัน
//   (ตาราง x Magnus ที่ค่า raw และส่วนต่างอุณหภูมิของแต่ละช่อง) ระดับ aw ของช่องนี้ = aw ที่ 25 °C ที่ทำนายได้
//   ตัวคูณถูกใส่ที่จุด aw สองจุดที่คร่อมระดับนั้นใน SURF_AW_GRID (จุดอื่นคงค่าเดิม) จึงบันทึกซ้ำด้วยสารมาตรฐานคนละตัวเพื่อต่อยอดพื้นผิวได้
// หมายเหตุ: ใช้ได้เมื่อสารมาตรฐานมี aw คงที่ในช่วง 19-25 °C (เช่น NaCl เปลี่ยนน้อยกว่า ~0.3%) — ค่าที่เกินเกณฑ์ SURF_K_MIN/MAX ถูกปฏิเสธ
bool commitSurfaceFromAutoCal(String& msg) {
#if AW_RAW_MODE
  msg = "RAW mode - surface disabled"; return false;
#else
  if (autoCalPhase != ACAL_DONE) { msg = "run all 10 auto-cal rounds first (status must be done)"; return false; }
  if (acalPointN[0] < 1 || isnan(acalPointRaw[0]) || isnan(acalSlotTempC[0])) { msg = "no 25 C reference point"; return false; }
  float aw25 = calTableLookup(acalPointRaw[0]) * acalSlotGf[0];
  if (aw25 <= 0.05f || aw25 >= 1.0f) { msg = "reference level out of range"; return false; }
  float newK[SURF_T_N][SURF_AW_N];
  for (int i = 0; i < SURF_T_N; i++) for (int j = 0; j < SURF_AW_N; j++) newK[i][j] = surfK[i][j];   // ต่อยอดจากพื้นผิวเดิม
  const int slotOfNode[SURF_T_N] = { 2, 1 };          // node 19 °C <- ช่อง 2, node 20 °C <- ช่อง 1
  int used = 0;
  char detail[160]; detail[0] = 0;
  for (int i = 0; i < SURF_T_N; i++) {
    int sl = slotOfNode[i];
    if (acalPointN[sl] < 1 || isnan(acalPointRaw[sl]) || isnan(acalSlotTempC[sl])) continue;
    if (fabsf(acalSlotTempC[sl] - SURF_T_NODE[i]) > 0.6f) { msg = "slot temperature too far from its node"; return false; }
    float awT = calTableLookup(acalPointRaw[sl]) * acalSlotGf[sl];
    if (awT <= 0.05f) { msg = "invalid point at low temperature"; return false; }
    float k = aw25 / awT;
    if (k < SURF_K_MIN || k > SURF_K_MAX) { msg = "factor out of accepted range - data rejected"; return false; }
    int j = 1;
    while (j < SURF_AW_N - 1 && aw25 > SURF_AW_GRID[j]) j++;   // j = จุดบนของช่วงที่คร่อมระดับ aw
    newK[i][j] = k;
    if (j - 1 >= 1) newK[i][j - 1] = k;                          // จุด aw = 0 ไม่แตะ
    size_t L = strlen(detail);
    snprintf(detail + L, sizeof(detail) - L, "%sk(%.0fC)=%.4f", L ? " " : "", (double)SURF_T_NODE[i], (double)k);
    used++;
  }
  if (used == 0) { msg = "no 20/19 C points available"; return false; }
  for (int i = 0; i < SURF_T_N; i++) for (int j = 0; j < SURF_AW_N; j++) surfK[i][j] = newK[i][j];
  surfValid = true;
  surfCommitCount++;
  saveCalSurface();
  char m[220];
  snprintf(m, sizeof(m), "surface saved at aw~%.3f: %s", (double)aw25, detail);
  msg = m;
  Serial.printf("[SURF] %s\n", m);
  return true;
#endif
}
void handleAdminSurfaceCommit() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  String msg;
  bool ok = commitSurfaceFromAutoCal(msg);
  server.send(ok ? 200 : 400, "text/plain", msg);
}
void handleAdminSurfaceClear() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  surfResetToUnity();
  surfCommitCount = 0;
  saveCalSurface();
  Serial.println("[SURF] surface cleared (factors = 1.0)");
  server.send(200, "text/plain", "surface cleared");
}
void handleAdminSurfaceStatus() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  String j = String("{\"active\":") + (surfValid ? "true" : "false") + ",\"commits\":" + String((unsigned long)surfCommitCount) + ",\"awGrid\":[";
  for (int a = 0; a < SURF_AW_N; a++) { if (a) j += ","; j += String(SURF_AW_GRID[a], 2); }
  j += "],\"rows\":[";
  for (int i = 0; i < SURF_T_N; i++) {
    if (i) j += ",";
    j += "{\"tempC\":" + String(SURF_T_NODE[i], 0) + ",\"k\":[";
    for (int a = 0; a < SURF_AW_N; a++) { if (a) j += ","; j += String(surfK[i][a], 4); }
    j += "]}";
  }
  j += "],\"nowFactor\":" + String(surfaceFactor(isnan(currentAw) ? 0.5f : currentAw, currentTempC), 4) + "}";
  server.send(200, "application/json", j);
}

// v-web-ctrl: GET /admin/calmode/status — แอดมินเท่านั้น สถานะสดของโหมดคาลิเบตอัตโนมัติ + ผลแต่ละรอบที่เสร็จแล้ว
void handleAdminCalStatus() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  const char* phaseStr = (autoCalPhase == ACAL_IDLE) ? "idle" : (autoCalPhase == ACAL_COOLING) ? "cooling" : (autoCalPhase == ACAL_MEASURING) ? "measuring" : "done";
  unsigned long elapsedSec = (autoCalPhase == ACAL_MEASURING) ? (millis() - autoCalPhaseStartMs) / 1000UL : 0UL;
  float showTargetC = (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) ? TARGET_TEMP_C_DEFAULT : TARGET_TEMP_C;
  int roundHuman = (autoCalPhase == ACAL_DONE) ? AUTOCAL_ROUNDS : (autoCalRound + 1);
  // v-led: ให้เว็บรู้สถานะไฟเดียวกับที่โชว์บนตัวเครื่อง (updateAutoCalLED()) จะได้ขึ้นจุดสีเดียวกันบนแดชบอร์ดด้วย
  // ถ้าอยากทำ — "red"/"yellow"/"green" ความหมายตรงกับไฟจริงทุกประการ (ดูคำอธิบายที่ updateAutoCalLED())
  const char* ledStr = "off";
  if (autoCalPhase == ACAL_COOLING) ledStr = peltierStuckHot ? "red" : "yellow";
  else if (autoCalPhase == ACAL_MEASURING) {
    int hp = computeAutoCalHoldPhase();
    ledStr = (hp == 2) ? "green" : (hp == 1) ? "yellow" : "red";
  }

  String json = "{";
  json += "\"phase\":\"" + String(phaseStr) + "\",";
  json += "\"led\":\"" + String(ledStr) + "\",";
  json += "\"round\":" + String(roundHuman) + ",";
  json += "\"totalRounds\":" + String(AUTOCAL_ROUNDS) + ",";
  json += "\"targetC\":" + String(showTargetC, 1) + ",";
  json += "\"currentTempC\":" + String(isnan(currentTempC) ? -99.0f : currentTempC, 1) + ",";
  json += "\"elapsedSec\":" + String(elapsedSec) + ",";
  json += "\"roundDurationSec\":" + String(AUTOCAL_MAX_ROUND_MS / 1000UL) + ",";   // v16: = เพดานเวลาต่อรอบ (จบเมื่อนิ่ง ไม่ใช่เวลาตายตัว)
  json += "\"maxRoundSec\":" + String(AUTOCAL_MAX_ROUND_MS / 1000UL) + ",";
  json += "\"verifyTolAw\":" + String(AUTOCAL_VERIFY_TOL_AW, 4) + ",";
  json += "\"isVerify\":" + String(((autoCalPhase == ACAL_DONE) ? false : autoCalIsVerifyRound(autoCalRound)) ? "true" : "false") + ",";
  json += "\"holdPhase\":" + String((autoCalPhase == ACAL_MEASURING) ? computeAutoCalHoldPhase() : 0) + ",";
  json += "\"pwmSteady\":" + String((autoCalPhase == ACAL_MEASURING && acalPwmSteadyWin) ? "true" : "false") + ",";   // v18
  json += "\"points\":[";
  for (int p = 0; p < AUTOCAL_SLOTS; p++) {
    static const float slotTemp[AUTOCAL_SLOTS] = { 25.0f, 20.0f, 19.0f };
    if (p) json += ",";
    json += "{\"tempC\":" + String(slotTemp[p], 1) + ",\"n\":" + String(acalPointN[p]) + ",\"raw\":" +
            (isnan(acalPointRaw[p]) ? String("null") : String(acalPointRaw[p], 5)) + "}";
  }
  json += "],";
  // v-acal-graph: ค่าเป้าหมาย (aw อ้างอิง) ที่กรอกไว้ตอนเริ่ม — null ถ้ายังไม่เคยเริ่มเลยตั้งแต่เปิดเครื่อง
  if (isnan(autoCalRefAw)) json += "\"refAw\":null,"; else json += "\"refAw\":" + String(autoCalRefAw, 4) + ",";
  // v-acal-graph: จุดกราฟสด (aw คาลิเบรตแล้ว) ของรอบที่กำลังวัดอยู่ ให้เว็บวาดกราฟเดียวกับที่โชว์บนจอเครื่อง
  json += "\"graphAw\":[";
  for (int g = 0; g < autoCalGraphCount; g++) {
    if (g) json += ",";
    json += String(autoCalGraphBuf[g], 4);
  }
  json += "],";
  json += "\"results\":[";
  for (int i = 0; i < AUTOCAL_ROUNDS; i++) {
    if (i) json += ",";
    json += "{\"round\":" + String(i + 1) + ",\"targetC\":" + String(AUTOCAL_TARGET_C[i], 1) + ",\"type\":\"" + String(autoCalIsVerifyRound(i) ? "test" : "measure") + "\",\"done\":";
    if (autoCalResults[i].sampleCount > 0) {
      json += "true,\"avgRaw\":" + String(autoCalResults[i].avgRawFrac, 4) +
              ",\"avgAw\":" + String(autoCalResults[i].avgAw, 4) +
              ",\"avgTempC\":" + String(autoCalResults[i].avgTempC, 2) +
              ",\"errAw\":" + (isnan(autoCalResults[i].errAw) ? String("null") : String(autoCalResults[i].errAw, 4)) +
              ",\"pass\":" + String(autoCalResults[i].passed ? "true" : "false") +
              ",\"fixed\":" + String(autoCalResults[i].fixed ? "true" : "false") +
              ",\"timedOut\":" + String(autoCalResults[i].timedOut ? "true" : "false") +
              ",\"pointRaw\":" + String(autoCalResults[i].pointRaw, 5) +
              ",\"durSec\":" + String((unsigned long)autoCalResults[i].durSec) + "}";
    } else {
      json += "false,\"avgRaw\":null,\"avgAw\":null,\"avgTempC\":null}";
    }
  }
  json += "]}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleRoot() {
  if (!checkAuth()) return;
  server.send_P(200, "text/html", index_html);
}

// v-pro: GET /clocksync?epoch=<unix seconds> — เบราว์เซอร์เรียกอัตโนมัติตอนเปิดหน้าเว็บ (ดู JS ใน index_html)
// เพื่อซิงก์ "เวลาโลกจริง" ให้เครื่อง แทนที่จะปล่อยให้ audit trail อ้างอิงแค่ millis() ที่รีเซ็ตทุกครั้งไฟดับ
void handleClockSync() {
  if (!checkAuth()) return;
  if (!server.hasArg("epoch")) { server.send(400, "text/plain", "missing epoch"); return; }
  time_t ep = (time_t)server.arg("epoch").toInt();
  if (ep < 1700000000) { server.send(400, "text/plain", "epoch looks invalid"); return; } // กันค่าพลาด/เบราว์เซอร์นาฬิกาผิด
  syncClockFromWeb(ep);
  server.send(200, "text/plain", "OK");
}

// v-pro: GET /setop?name=<initials> — ตั้งชื่อย่อผู้ปฏิบัติงานปัจจุบัน แนบไปกับทุกค่าที่บันทึกจากนี้ (audit trail)
void handleSetOperator() {
  if (!checkAuth()) return;
  if (!server.hasArg("name")) { server.send(400, "text/plain", "missing name"); return; }
  saveOperatorTag(server.arg("name").c_str());
  server.send(200, "text/plain", "OK");
}

// v13: ใส่ข้อความเข้า JSON อย่างปลอดภัย (ตัด " \ และอักขระควบคุม) — ชื่อผู้ปฏิบัติงานที่ผู้ใช้พิมพ์เองเคยทำให้ JSON พังได้
void jsonEscapeInto(const char* in, char* out, size_t outSize) {
  size_t o = 0;
  for (size_t i = 0; in && in[i] && o + 1 < outSize; i++) {
    char c = in[i];
    if (c == '"' || c == '\\' || (unsigned char)c < 0x20) c = '_';
    out[o++] = c;
  }
  out[o] = '\0';
}

void handleData() {
  if (!checkAuth()) return;
  // v13: ถ้ารอบวัดบนเครื่องเพิ่งอ่าน SHT สำเร็จภายใน ~1 วิ ใช้ค่านั้นเลย (ไม่อ่านซ้ำ) — ลดทราฟฟิก I2C ที่ซ้อนกับรอบวัด
  // และไม่ต้องรอ I2C ในจังหวะที่กำลังตอบเว็บ ถ้าค่าเก่าเกินไปค่อยอ่านใหม่ (พร้อมลองซ้ำ 1 ครั้งเมื่อ CRC ผิด)
  float rh;
  bool rhReadOk;
  if (!isnan(lastShtRhPct) && millis() - lastShtReadMs < 900UL) {
    rh = lastShtRhPct;
    rhReadOk = true;
  } else {
    rh = readShtHumidityRetry();
    // v-fix: เดิมถ้าอ่านค่าจาก SHT ไม่ได้ (rh เป็น NaN เพราะสายหลวม/I2C แฮงก์ชั่วขณะ) จะบังคับ rh = 0.0 ทำให้กราฟบนเว็บ
    // ตกฮวบไปที่ 0.00 — แก้ให้ใช้ %RH ล่าสุดที่อ่านได้สำเร็จแทน (lastGoodRhWeb) พร้อมยกธง faultSHT ให้เว็บแจ้งเตือน
    rhReadOk = (!isnan(rh) && rh >= 0.0 && rh <= 100.0);
    if (rhReadOk) {
      lastShtRhPct = rh;
      lastShtReadMs = millis();
      shtFailStreak = 0;
    } else {
      rh = lastGoodRhWeb;
      if (++shtFailStreak >= 3) shtBusRecover();
    }
  }
  if (rhReadOk) lastGoodRhWeb = rh;

  float elapsedMin = (millis() - dashboardStartTime) / 60000.0;
  float tempOut = isnan(currentTempC) ? -99.0 : currentTempC;
  float rawAwOut = rh / 100.0; // ค่าดิบก่อนผ่านสมการคาลิเบรต (เศษส่วน 0-1) ใช้เทียบกับ aw ที่คาลิเบรตแล้วบนเว็บ
  float roomRawOut = (isnan(ambientRHatBoot) || ambientRHatBoot < 0.0f || ambientRHatBoot > 100.0f)
                   ? -1.0f : constrain(ambientRHatBoot / 100.0f, 0.0f, 1.0f);
  float roomAwOut = roomReferenceAw(currentTempC);

  // v14: ทั้งสองโหมดคำนวณ aw สด ๆ ทุกครั้งที่ /data ถูกเรียก — โหมด RAW = RH/100 ตรง ๆ, โหมดคาลิเบรต = ตารางคาลิเบรต + ชดเชยอุณหภูมิ
  // (เดิมโหมดคาลิเบรตใช้ currentAw ที่ขยับเฉพาะตอนกดวัดที่ตัวเครื่อง ทำให้หน้าเว็บค้างค่าเก่าเมื่อไม่ได้วัด; ค่า "ผลวัดที่นิ่งแล้ว"
  // ที่ใช้ตัดสินบันทึก/ป็อปอัปยังคงมาจากรอบวัดบนเครื่องเหมือนเดิม)
  float gfNow = AW_RAW_MODE ? 1.0f : gradientFactor();
  float dTnow = gradientDeltaC();
  // v-raw-idle: ค่าสดบนเว็บ = ค่าดิบ จนกว่าจะกดวัด/เริ่มคาลิเบตอัตโนมัติ
  bool calibratedNow = substanceMeasuringNow() || autoCalActive();
  float awOut = calibratedNow ? applyCal(rawAwOut, currentTempC) : constrain(rawAwOut, 0.0f, 1.0f);
  float toBaseOut = awOut, toOffOut = 0.0f, toTgtOut = awOut, toZOut = 0.0f, toWOut = 0.0f, toSlOut = 0.0f;  // v-trend-offset
  float toEqRawOut = NAN; bool toEqUsedOut = false;  // v2: จุดสมดุลที่ทำนาย (ดู fitAR1 ใน applySubstanceGuessBias)
#if !AW_RAW_MODE
  // v29: อ่านค่าที่ลูปวัดบนเครื่องคำนวณไว้ (ทางเดียวกับที่จอเครื่องแสดง) แทนการคำนวณ/เดินสถานะเองตอนโพล
  if (substanceMeasuringNow() && !isnan(toShown) && (millis() - toShownMs) < 5000UL) {
    awOut = toShown;
    toBaseOut = toBase; toOffOut = toOff; toTgtOut = isnan(toTarget) ? toBase : toTarget;
    toZOut = toZ; toWOut = toW; toSlOut = toSlope; toEqRawOut = toEqRaw; toEqUsedOut = toEqUsed;
  }
#endif
  float roomDeltaAwOut = (!isnan(roomAwOut) && !isnan(awOut)) ? (awOut - roomAwOut) : 0.0f;

  // v13: สร้าง JSON ด้วย snprintf ลงบัฟเฟอร์คงที่ แทนการต่อ String ~30 ครั้งทุกวินาที (ทำให้ heap แตกเป็นเศษเล็ก ๆ
  // เมื่อเปิดเครื่องทิ้งไว้นานหลายวัน -> จัดสรรหน่วยความจำไม่ได้ในที่สุด -> รีบูต)
  char opSafe[12];
  jsonEscapeInto(operatorTag, opSafe, sizeof(opSafe));
  bool calDueNow = (clockEverSynced && calSavedAtEpoch > 0 && (nowEpoch() - calSavedAtEpoch) / 86400L >= CAL_REMINDER_DAYS);
  long calAgeNow = (clockEverSynced && calSavedAtEpoch > 0) ? (long)((nowEpoch() - calSavedAtEpoch) / 86400L) : -1L;
  // v-dist: อ่านสัญญาณของอุปกรณ์ที่เชื่อมต่อ AP อยู่ตอนนี้ (ถ้ามี) แปลงเป็นระยะทางโดยประมาณ ให้เว็บโชว์ "ระยะห่างจากตัวเครื่อง"
  int8_t clientRssiNow = 0;
  bool clientConnectedNow = getClientRssi(&clientRssiNow);
  float clientDistNow = clientConnectedNow ? rssiToDistanceM(clientRssiNow) : 0.0f;

  static char json[2200];   // v-room-aw: เพิ่มพื้นที่รองรับ roomAw/roomRaw/roomDeltaAw จากเดิม 1900
  snprintf(json, sizeof(json),
    "{\"aw\":%.5f,\"raw\":%.6f,\"roomAw\":%.5f,\"roomRaw\":%.6f,\"roomDeltaAw\":%.5f,\"rh\":%.4f,\"temp\":%.1f,\"t\":%.3f,\"heater\":%s,"
    // v12: เฟสของเครื่อง (idle/preheat/precool/measuring/postheat/postcool) และสถานะป็อปอัป "บันทึกไหม"
    "\"phase\":\"%s\",\"savePrompt\":%s,"
    // v-stability: ส่งสถานะสุขภาพระบบไปด้วย ให้เว็บแดชบอร์ดแจ้งเตือนผู้ใช้ได้ทันทีเมื่อเซนเซอร์มีปัญหา
    "\"faultSHT\":%s,\"faultTemp\":%s,\"roomTooHot\":%s,"
    // v-pro: สถานะ audit trail/System Health
    "\"clockSynced\":%s,\"clockVerified\":%s,\"operator\":\"%s\",\"noiseWarning\":%s,\"calAgeDays\":%ld,\"calDue\":%s,"
    // v13: เครื่องอยู่ในโหมด SAFE START (รีเซ็ตผิดปกติติดกัน) หรือไม่
    "\"safeStart\":%s,"
    // v14: อุณหภูมิตัวชิป SHT, ส่วนต่างชิป-ตัวอย่าง (°C), ตัวคูณชดเชย, ห้องเย็นกว่าเป้าหมายจนเทลเทียร์คุมไม่ได้
    "\"shtT\":%.1f,\"dT\":%.2f,\"gf\":%.4f,\"coldRoom\":%s,"
    // v-pid-tune: สถานะลูปควบคุมเทลเทียร์ ณ ขณะนี้ — ใช้วาดกราฟ/ตัวเลขสดในแผงจูน PID บนเว็บ (ไม่ผูกกับรอบวัด aw)
    "\"pwm\":%d,\"pwmPct\":%.1f,\"targetC\":%.1f,\"usingPid\":%s,\"kp\":%.3f,\"ki\":%.4f,\"kd\":%.3f,"
    // v15: true = ตัวตรวจแนวโน้มไหลช้า (slow-trend gate) กำลังยับยั้งไม่ให้ล็อกค่า "นิ่ง" อยู่ แม้หน้าต่างสั้นจะแบนแล้ว
    // (มักเกิดตอนกระโดดข้าม %RH ระหว่างตัวอย่างมาก เช่น น้ำ <-> เกลืออิ่มตัว) — เว็บใช้โชว์คำอธิบายกันผู้ใช้งง
    "\"stillDrifting\":%s,\"stabPhase\":%d,\"pwmSteady\":%s,"
    // v-led-web: สีไฟสถานะ (LED) จริงบนตัวเครื่อง ณ ขณะนี้ (off/red/green/blue/yellow/purple/cyan/white) + กำลังกระพริบอยู่หรือไม่
    // ให้เว็บวาดจุดสีเดียวกันเป๊ะ ไม่ต้องคอยเดา/คำนวณเองจากฟิลด์อื่น
    "\"ledColor\":\"%s\",\"ledBlink\":%s,"
    // v-dist: มีอุปกรณ์เชื่อมต่อ AP อยู่หรือไม่ (ถ้ามีหลายเครื่องเชื่อมต่อพร้อมกัน ใช้ตัวที่สัญญาณแรงสุด) + RSSI (dBm) และระยะห่างโดยประมาณ (เมตร)
    "\"clientConnected\":%s,\"clientRssi\":%d,\"clientDistM\":%.1f,"
    // v-trend-offset: ค่าที่ใช้คำนวณ offset อัตโนมัติ ณ ขณะนี้ (ให้กราฟ Offset ของแอดมินวาดสูตร/เส้นประ/error)
    "\"toBase\":%.4f,\"toOff\":%.4f,\"toTgt\":%.4f,\"toZ\":%.2f,\"toW\":%.2f,\"toSl\":%.4f,"
    // v2: จุดสมดุลที่ทำนายจาก fitAR1() (-1 = ยังทำนายไม่ได้/ไม่ได้ใช้ — เว็บอ่านค่า < 0 เป็น "ไม่มีข้อมูล")
    "\"toEqRaw\":%.4f,\"toEqUsed\":%s}",
    (double)awOut, (double)rawAwOut, (double)(isnan(roomAwOut) ? -1.0f : roomAwOut), (double)roomRawOut,
    (double)roomDeltaAwOut, (double)rh, (double)tempOut, (double)elapsedMin,
    sensorHeaterOn ? "true" : "false",
    sensorPhaseName(), (savePromptActive && state != ST_SENSOR_COND) ? "true" : "false",
    (sensorFaultSHT || !rhReadOk) ? "true" : "false", sensorFaultDS18B20 ? "true" : "false", peltierStuckHot ? "true" : "false",
    clockEverSynced ? "true" : "false", clockVerifiedThisBoot ? "true" : "false", opSafe,
    lastMeasureNoiseWarning ? "true" : "false", calAgeNow, calDueNow ? "true" : "false",
    safeStart ? "true" : "false",
    (double)(isnan(shtTempC) ? -99.0f : shtTempC), (double)(isnan(dTnow) ? 0.0f : dTnow), (double)gfNow, coldRoomWarn ? "true" : "false",
    peltierOutputPWM, (double)(peltierOutputPWM * 100.0f / 255.0f), (double)TARGET_TEMP_C,
    (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND) ? "true" : "false",
    (double)pid_Kp, (double)pid_Ki, (double)pid_Kd,
    trendStillDrifting ? "true" : "false",
    (state == ST_MEASURE_AW || state == ST_COMPARE_MEASURE) ? stabPhase : 0,   // v16: สีไฟ 0 แดง / 1 เหลือง / 2 เขียว ตรงกับ LED บนเครื่อง
    // v18: PWM เทลเทียร์นิ่งตลอด 60 วิล่าสุดหรือไม่ (เว็บใช้ผ่อนเกณฑ์ป็อปอัป "ค่าคงที่แล้ว" ให้ตรงกับเครื่อง)
    (autoCalPhase == ACAL_MEASURING) ? (acalPwmSteadyWin ? "true" : "false")
      : ((state == ST_MEASURE_AW || state == ST_COMPARE_MEASURE) && stabPwmSteadyNow) ? "true" : "false",
    ledColorName(), ledRepBlink ? "true" : "false",
    clientConnectedNow ? "true" : "false", (int)clientRssiNow, (double)clientDistNow,
    (double)toBaseOut, (double)toOffOut, (double)toTgtOut, (double)toZOut, (double)toWOut, (double)toSlOut,
    (double)(isnan(toEqRawOut) ? -1.0f : toEqRawOut), toEqUsedOut ? "true" : "false");

  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// v11: ส่งข้อมูลอุปกรณ์/เซนเซอร์/สมการคาลิเบรตปัจจุบัน ให้หน้าเว็บดึงไปแนบเป็นเมทาดาทาหัวไฟล์ทุกครั้งที่
// ส่งออกกราฟ/CSV เพื่อให้ไฟล์ข้อมูลตรวจสอบย้อนกลับได้ครบถ้วน (ดูคำอธิบายที่จุดประกาศตัวแปรด้านบนไฟล์)
void handleDeviceInfo() {
  if (!checkAuth()) return;
  String cal = "[";
  for (int i = 0; i < CAL_POINTS_COUNT; i++) {
    if (i) cal += ",";
    cal += "{\"raw\":" + String(calPoints[i].raw, 3) + ",\"aw\":" + String(calPoints[i].aw, 3) + "}";
  }
  cal += "]";
#if AW_RAW_MODE
  cal = "[]";  // RAW: ไม่ได้ใช้จุดคาลิเบรต จึงไม่แนบไปกับไฟล์ export
#endif
  char calSavedBuf[20], nowBuf[20];
  formatEpoch(calSavedAtEpoch, calSavedBuf, sizeof(calSavedBuf));
  formatEpoch(nowEpoch(), nowBuf, sizeof(nowBuf));
  String json = "{";
  json += "\"fwVersion\":\"" + String(FW_VERSION) + "\",";
  json += "\"deviceName\":\"" + String(ssid) + "\",";
  json += "\"valueMode\":\"" + String(VALUE_MODE) + "\",";
  json += "\"humiditySensor\":\"" + String(sensorModelHumidity()) + "\",";
  json += "\"humiditySensorType\":\"" + String(humSensorTypeName()) + "\",";
  json += "\"tempSensor\":\"" + String(SENSOR_MODEL_TEMP) + "\",";
  json += "\"targetTempC\":" + String(TARGET_TEMP_C, 1) + ",";
  json += "\"calTempMinC\":" + String(CAL_TEMP_MIN_C, 1) + ",";
  json += "\"calTempMaxC\":" + String(CAL_TEMP_MAX_C, 1) + ",";
  json += "\"stabWindowSec\":" + String(STAB_WINDOW_MS / 1000UL) + ",";
  // v12: เกณฑ์ "กราฟนิ่ง" ที่ใช้เด้งป็อปอัปบันทึก = ช่วงกว้างของเส้น aw บนกราฟ <= SAVE_PROMPT_TOL ใน savePromptWindowPts() จุดล่าสุด
  json += "\"stabToleranceAw\":" + String(SAVE_PROMPT_TOL, 4) + ",";
  json += "\"stabGraphWindowPts\":" + String(savePromptWindowPts()) + ",";
  json += "\"stabGraphWindowSec\":" + String((savePromptWindowPts() * updateInterval) / 1000.0, 1) + ",";
  // v12: โปรแกรมฮีตเตอร์เซนเซอร์ SHT ก่อน/หลังวัด แนบไปกับใบรายงานเป็นเงื่อนไขการทดสอบ
  json += "\"shtPreHeatSec\":" + String(SHT_PREHEAT_MS / 1000UL) + ",";
  json += "\"shtPreCoolSec\":" + String(SHT_PRECOOL_MS / 1000UL) + ",";
  json += "\"shtPostHeatSec\":" + String(SHT_POSTHEAT_MS / 1000UL) + ",";
  json += "\"shtPostCoolSec\":" + String(SHT_POSTCOOL_MS / 1000UL) + ",";
  json += "\"minMeasureDurationSec\":" + String(MIN_MEASURE_DURATION_MS / 1000UL) + ",";
  json += "\"stabYellowSec\":" + String(STAB_YELLOW_MS / 1000UL) + ",";                 // v16: คงที่กี่วิถึงขึ้นเหลือง
  json += "\"stabSlopeMaxPerMin\":" + String(STAB_SLOPE_MAX_PER_MIN, 5) + ",";         // v16: ความชันสูงสุด (aw/นาที) ที่ถือว่านิ่ง
  // v-pro: เมทาดาทา audit trail แนบไปกับ export ทุกครั้ง — เวลาปัจจุบัน (ตาม nowEpoch()), ผู้ปฏิบัติงาน,
  // และเวลาที่คาลิเบรตครั้งล่าสุด เพื่อให้ไฟล์ที่ export ออกไปตรวจสอบย้อนกลับได้ครบถ้วนกว่าเดิม
  // v13: ข้อมูลความเสถียรของเครื่อง (สาเหตุรีเซ็ตล่าสุด/จำนวนครั้ง, การกู้บัส I2C, ช่อง Wi-Fi) ให้หน้าเว็บโชว์ใน System Health
  json += "\"gradCorr\":" + String(TEMP_GRADIENT_CORRECTION ? "true" : "false") + ",";
  json += "\"gradDeadbandC\":" + String(gradientDeadbandC(), 2) + ",";
  json += "\"learnedOffsetSdC\":" + String(isnan(learnedOffsetSdC) ? -1.0f : learnedOffsetSdC, 2) + ",";
  json += "\"shtOffsetC\":" + String(SHT_MINUS_DS_OFFSET_C, 2) + ",";
  // Phase 2: offset ที่เรียนรู้จริงจากหลายบูต (ถ้ายังไม่เชื่อถือได้ effectiveOffsetC จะเท่ากับ shtOffsetC ด้านบน)
  json += "\"learnedOffsetC\":" + String(isnan(learnedShtDsOffsetC) ? -999.0f : learnedShtDsOffsetC, 2) + ",";
  json += "\"learnedOffsetSamples\":" + String((unsigned long)learnedOffsetSampleCount) + ",";
  json += "\"learnedOffsetTrusted\":" + String(learnedOffsetTrusted ? "true" : "false") + ",";
  json += "\"effectiveOffsetC\":" + String(getEffectiveShtDsOffsetC(), 2) + ",";
  // Phase 1: ค่าความชื้น/อุณหภูมิห้องที่จับได้ตอนบูต (ambientBootValid=false = ค่าค้างจากบูตรอบก่อน ไม่ใช่รอบนี้)
  json += "\"ambientRHatBootPct\":" + String(isnan(ambientRHatBoot) ? -1.0f : ambientRHatBoot, 1) + ",";
  json += "\"ambientTempCatBoot\":" + String(isnan(ambientTempCatBoot) ? -999.0f : ambientTempCatBoot, 2) + ",";
  json += "\"ambientBootValid\":" + String(ambientBootValid ? "true" : "false") + ",";
  json += "\"lastReset\":\"" + String(resetReasonName(lastResetReason)) + "\",";
  json += "\"brownoutCount\":" + String(bootBrownoutCount) + ",";
  json += "\"wdtCount\":" + String(bootWdtCount) + ",";
  json += "\"panicCount\":" + String(bootPanicCount) + ",";
  json += "\"safeStart\":" + String(safeStart ? "true" : "false") + ",";
  json += "\"i2cRecoveries\":" + String((unsigned long)(i2cRecoverCount + shtRecoverCount)) + ",";
  json += "\"lcdReinits\":" + String((unsigned long)lcd.reinitCount) + ",";
  json += "\"wifiChannel\":" + String((int)wifiChannelInUse) + ",";
  json += "\"wifiRestarts\":" + String((unsigned long)wifiRestartCount) + ",";
  json += "\"exportedAtUtc\":\"" + String(nowBuf) + "\",";
  json += "\"clockVerifiedThisBoot\":" + String(clockVerifiedThisBoot ? "true" : "false") + ",";
  json += "\"operator\":\"" + String(operatorTag) + "\",";
  json += "\"calibratedAtUtc\":\"" + String(calSavedBuf) + "\",";
  json += "\"calPoints\":" + cal;
  json += "}";
  server.send(200, "application/json", json);
}

// v-accuracy: GET /calpoints -> อ่านจุดคาลิเบรตที่ใช้งานอยู่ตอนนี้ + ค่าโรงงาน ให้หน้าเว็บโชว์ในตาราง
void handleCalGet() {
  if (!checkAuth()) return;
  String json = "{\"points\":[";
  for (int i = 0; i < CAL_POINTS_COUNT; i++) {
    if (i) json += ",";
    json += "{\"raw\":" + String(calPoints[i].raw, 4) + ",\"aw\":" + String(calPoints[i].aw, 4) + "}";
  }
  json += "],\"factory\":[";
  for (int i = 0; i < CAL_POINTS_COUNT; i++) {
    if (i) json += ",";
    json += "{\"raw\":" + String(CAL_POINTS_FACTORY[i].raw, 4) + ",\"aw\":" + String(CAL_POINTS_FACTORY[i].aw, 4) + "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// v-accuracy: GET /calset?p0raw=..&p0aw=..&p1raw=..&p1aw=.. (ตามจำนวน CAL_POINTS_COUNT) — บันทึกจุดคาลิเบรต
// ใหม่ลง NVS แล้วใช้งานทันที ปฏิเสธถ้ากำลังวัดค่าอยู่ (กันแก้คาลิเบรตกลางคันจนค่าที่กำลังวัดกระโดด) หรือข้อมูลไม่ผ่านตรวจสอบ
void handleCalSet() {
  if (!checkAuth()) return;
#if AW_RAW_MODE
  server.send(403, "text/plain", "RAW firmware: calibration is disabled");
  return;
#endif
  if (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND) {
    server.send(409, "text/plain", "Cannot change calibration while a measurement is in progress");
    return;
  }
  CalPoint newPts[CAL_POINTS_COUNT];
  for (int i = 0; i < CAL_POINTS_COUNT; i++) {
    String rk = "p" + String(i) + "raw", ak = "p" + String(i) + "aw";
    if (!server.hasArg(rk) || !server.hasArg(ak)) {
      server.send(400, "text/plain", "missing argument for point " + String(i));
      return;
    }
    newPts[i].raw = server.arg(rk).toFloat();
    newPts[i].aw = server.arg(ak).toFloat();
  }
  String err;
  if (!saveCalPoints(newPts, CAL_POINTS_COUNT, err)) {
    server.send(400, "text/plain", err);
    return;
  }
  server.send(200, "text/plain", "OK");
}

// v-accuracy: GET /calreset -> กลับไปใช้ค่าโรงงาน (CAL_POINTS_FACTORY) ทันที เผื่อผู้ใช้ปรับแล้วพลาด/อยากเริ่มใหม่
void handleCalReset() {
  if (!checkAuth()) return;
#if AW_RAW_MODE
  server.send(403, "text/plain", "RAW firmware: calibration is disabled");
  return;
#endif
  if (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND) {
    server.send(409, "text/plain", "Cannot change calibration while a measurement is in progress");
    return;
  }
  String err;
  saveCalPoints(CAL_POINTS_FACTORY, CAL_POINTS_COUNT, err);
  server.send(200, "text/plain", "OK");
}

// ============================================================================
//  v22: คาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก ("Quick Cal") — ตามคำขอ "คาลิเบตตามค่าที่สอบเทียบได้เพื่อประหยัดเวลา
//  แต่ยังใช้คาลิเบตอันก่อน ๆ ร่วมด้วย" — ผู้ใช้มีเครื่องคาลิเบรตอ้างอิงอยู่แล้ว อ่านค่า aw จากเครื่องนั้นตรง ๆ แล้วป้อน
//  เข้าเว็บของเครื่องนี้ทันที (ไม่ต้องรอ AUTOCAL_ROUNDS ครบ 10 รอบ x สูงสุด 30 นาที/รอบ) แต่แทนที่จะ "เขียนทับ" ตาราง
//  คาลิเบรตทั้งชุด จะ "ผสม" ค่าอ้างอิงใหม่เข้ากับจุดเดิมที่ใกล้เคียง raw ปัจจุบัน (ถ่วงน้ำหนัก QUICKCAL_BLEND_WEIGHT
//  และลดอิทธิพลลงตามระยะห่าง raw — จุดที่ไกลออกไปแทบไม่ถูกแตะ) จึงยังคง "ใช้คาลิเบตอันก่อนหน้าร่วมด้วย" ตามที่ขอ
//  ไม่ใช่ล้างของเก่าทิ้งทั้งตารางจากตัวอย่างเดียว
// ============================================================================
const float QUICKCAL_BLEND_WEIGHT = 0.6f;   // 0-1: ยิ่งใกล้ 1 ยิ่งเชื่อค่าอ้างอิงใหม่มาก / ยิ่งใกล้ 0 ยิ่งเชื่อตารางเดิมมาก
const float QUICKCAL_INFLUENCE_RAW = 0.08f; // ระยะ raw (หน่วยเดียวกับ calPoints[].raw คือ 0-1) ที่ถือว่า "ใกล้" — คุมว่าการแก้ไขกระจายไปกว้างแค่ไหน

// GET /calquick?refAw=0.xxxx — อ่านค่าดิบสดจากเซนเซอร์ ณ ตอนนี้ เทียบกับค่าอ้างอิงที่ผู้ใช้พิมพ์มาจากเครื่องคาลิเบรต
// ภายนอก แล้วปรับจุดคาลิเบรตที่ raw ใกล้เคียงให้เข้าใกล้ค่าที่ควรจะเป็น (ผสมกับของเดิม ไม่ใช่แทนที่ทั้งหมด)
// v-cal-priority: ลำดับ 1) ของ 3 ลำดับความสำคัญการคาลิเบต (ดูคำอธิบายเต็มที่ applyCal()) — แก้ตาราง calPoints
// โดยตรงและถาวร ทำให้ทุกการอ่านค่าถัดไป (ลำดับ 3 คาลิเบตปกติ) ได้รับผลจาก Quick Cal นี้ไปด้วยเสมอ
void handleCalQuick() {
  if (!checkAuth()) return;
#if AW_RAW_MODE
  server.send(403, "text/plain", "RAW firmware: calibration is disabled");
  return;
#endif
  if (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND) {
    server.send(409, "text/plain", "Cannot quick-calibrate while a measurement is in progress");
    return;
  }
  if (autoCalPhase != ACAL_IDLE && autoCalPhase != ACAL_DONE) {
    server.send(409, "text/plain", "Cannot quick-calibrate while auto-cal is running");
    return;
  }
  if (!server.hasArg("refAw")) { server.send(400, "text/plain", "missing refAw"); return; }
  float refAw = server.arg("refAw").toFloat();
  if (isnan(refAw) || refAw < 0.0f || refAw > 1.0f) { server.send(400, "text/plain", "refAw out of range 0..1"); return; }

  float raw;
  readAwAndRaw(raw);   // อ่านค่าดิบสด ๆ จากเซนเซอร์จริงตอนนี้ (ค่าที่คาลิเบรตแล้วที่คืนมาไม่ได้ใช้ตรงนี้ ใช้ raw+ตารางเองแทน)
  if (raw < 0) { server.send(503, "text/plain", "sensor read failed"); return; }

  // เป้าหมายในสเกล "ก่อนชดเชยอุณหภูมิ" (ตารางเก็บค่าก่อนคูณ gradientFactor()) กันไม่ให้ชดเชยอุณหภูมิถูกนับซ้ำสองรอบ
  float gf = gradientFactor();
  if (gf < 0.01f || isnan(gf)) gf = 1.0f;
  float predictedTableAw = calTableLookup(raw);
  gf *= surfaceFactor(predictedTableAw * gf, currentTempC);   // v27: ตารางเก็บค่า "ก่อน" พื้นผิวอุณหภูมิ กันนับซ้ำ (=1.0 ถ้ายังไม่มีพื้นผิว)
  if (gf < 0.01f || isnan(gf)) gf = 1.0f;
  float targetTableAw = constrain(refAw / gf, 0.0f, 1.0f);
  float errAw = targetTableAw - predictedTableAw;

  CalPoint newPts[CAL_POINTS_COUNT];
  for (int i = 0; i < CAL_POINTS_COUNT; i++) newPts[i] = calPoints[i];

  // กระจายการแก้ไปยังจุดที่ raw ใกล้เคียงที่สุดก่อน (น้ำหนักลดลงตามระยะห่าง) จุดที่ไกลออกไปแทบไม่ถูกแตะ
  // -> ตารางเดิมยังอยู่ครบ ("ใช้คาลิเบตอันก่อนหน้าร่วมด้วย") ขยับเฉพาะช่วงที่เกี่ยวข้องกับค่าที่เพิ่งวัดจริง
  for (int i = 0; i < CAL_POINTS_COUNT; i++) {
    float d = fabsf(calPoints[i].raw - raw);
    float influence = 1.0f / (1.0f + (d / QUICKCAL_INFLUENCE_RAW));
    newPts[i].aw = constrain(calPoints[i].aw + errAw * influence * QUICKCAL_BLEND_WEIGHT, 0.0f, 1.0f);
  }
  // กันตารางใหม่ไขว้ลำดับกัน (raw คงเดิมเสมอ เปลี่ยนแค่ aw) เผื่อจุดใกล้กันขยับสวนทางจนค่า aw แซงกัน
  for (int i = 1; i < CAL_POINTS_COUNT; i++) {
    if (newPts[i].aw < newPts[i - 1].aw) newPts[i].aw = newPts[i - 1].aw;
  }

  String err;
  if (!saveCalPoints(newPts, CAL_POINTS_COUNT, err)) {
    server.send(400, "text/plain", "quick-cal rejected: " + err);
    return;
  }
  char resp[256];
  snprintf(resp, sizeof(resp),
    "{\"ok\":true,\"raw\":%.5f,\"refAw\":%.5f,\"predictedAwBefore\":%.5f,\"errAw\":%.5f}",
    (double)raw, (double)refAw, (double)constrain(predictedTableAw * gf, 0.0f, 1.0f), (double)errAw);
  server.send(200, "application/json", resp);
}

// v-pid-tune: GET /pidset?kp=..&ki=..&kd=.. — ปรับเกน PID ของเทลเทียร์ "สด" ระหว่างเครื่องทำงานอยู่ เพื่อให้จูนหน้าเว็บได้
// ทีละค่าโดยไม่ต้องแก้โค้ด/คอมไพล์/อัปโหลดเฟิร์มแวร์ใหม่ทุกครั้ง — ทุกครั้งที่ปรับ ค่าที่ใช้จริงจะไปโผล่ในฟิลด์
// kp/ki/kd ของ /data ทันที (เว็บ log ไว้พร้อมเวลา ใช้เป็นหลักฐานลำดับขั้นตอนการจูนให้กรรมการตรวจสอบย้อนหลังได้)
// จำกัดค่าไว้ในช่วงที่สมเหตุสมผลกันพิมพ์ผิดแล้วเทลเทียร์แกว่งรุนแรง/ไหม้ — ถ้าอยากได้ช่วงกว้างกว่านี้ค่อยแก้ตรงนี้
void handlePidSet() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; } // v-pid-tune: ควบคุมเทลเทียร์โดยตรง จำกัดแอดมินเท่านั้น เหมือนแผงคาลิเบตอัตโนมัติ
  if (!server.hasArg("kp") || !server.hasArg("ki") || !server.hasArg("kd")) {
    server.send(400, "text/plain", "missing kp/ki/kd");
    return;
  }
  float newKp = server.arg("kp").toFloat();
  float newKi = server.arg("ki").toFloat();
  float newKd = server.arg("kd").toFloat();
  if (isnan(newKp) || isnan(newKi) || isnan(newKd) ||
      newKp < 0 || newKp > 200 || newKi < 0 || newKi > 20 || newKd < 0 || newKd > 100) {
    server.send(400, "text/plain", "out of range (kp 0-200, ki 0-20, kd 0-100)");
    return;
  }
  pid_Kp = newKp;
  pid_Ki = newKi;
  pid_Kd = newKd;
  // ล้างค่าสะสม integral/derivative ทุกครั้งที่เปลี่ยนเกน กันไม่ให้ค่าอินทิกรัลที่คำนวณด้วยเกนชุดเก่าไปปนกับชุดใหม่
  // (ถ้าไม่ล้าง อาจเห็นเทลเทียร์กระตุกผิดธรรมชาติทันทีที่กดปรับ ซึ่งไม่ใช่พฤติกรรมจริงของเกนชุดใหม่)
  pid_Integral = 0;
  server.send(200, "text/plain", "OK");
}

// v20: GET /pidautotune?start=1 เริ่มจูน PID อัตโนมัติ (relay auto-tune) — ต้องเป็นแอดมิน เหมือน /pidset
// GET /pidautotune?cancel=1 ยกเลิกกลางคัน (กู้เกนเดิมคืน) — ดูรายละเอียดอัลกอริทึมที่ runPidAutoTuneStep()
void handlePidAutoTuneStart() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  if (server.hasArg("cancel")) {
    cancelPidAutoTune(true);
    server.send(200, "text/plain", "cancelled");
    return;
  }
  String err;
  if (!startPidAutoTune(err)) {
    server.send(409, "text/plain", "cannot start: " + err);
    return;
  }
  server.send(200, "text/plain", "started");
}

// v20: GET /pidautotune/status — โพลทุก ~1 วิระหว่างจูน เพื่อโชว์ความคืบหน้า/ผลลัพธ์บนแผงจูน PID บนเว็บ
void handlePidAutoTuneStatus() {
  if (!checkAuth()) return;
  int stepsDone = max(0, atuneHalfCycleCount - ATUNE_HALFCYCLES_SKIP);
  static char json[420];
  snprintf(json, sizeof(json),
    "{\"state\":%d,\"stepsDone\":%d,\"stepsNeeded\":%d,\"elapsedS\":%lu,\"envC\":%.1f,"
    "\"ku\":%.3f,\"pu\":%.1f,\"kp\":%.3f,\"ki\":%.4f,\"kd\":%.3f,\"reason\":\"%s\"}",
    (int)pidATuneState, stepsDone, ATUNE_HALFCYCLES_USE,
    (pidATuneState == ATUNE_RUNNING) ? (unsigned long)((millis() - atuneStartMs) / 1000UL) : 0UL,
    (double)atuneEnvC, (double)atuneKu, (double)atunePu,
    (double)pid_Kp, (double)pid_Ki, (double)pid_Kd, atuneFailReason.c_str());
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// ---- HTTP: GET /adv/status , GET /adv/set?kf=0|1&ifix=0|1&mpc=0|1  (แอดมิน) ----
void handleAdvStatus() {
  if (!checkAuth()) return;
  advInitOnce();
  static char json[1500];
  snprintf(json, sizeof(json),
    "{\"cfg\":{\"kf\":%d,\"ifix\":%d,\"mpc\":%d},"
    "\"kf\":{\"ok\":%d,\"T\":%.3f,\"dTdtPerMin\":%.3f,\"sd\":%.4f,\"nis\":%.2f,\"rej\":%d},"
    "\"ekf\":{\"init\":%d,\"aw\":%.4f,\"sdAw\":%.4f,\"awPerMin\":%.5f,\"sdAwPerMin\":%.5f,\"deltaC\":%.2f,\"sdDeltaC\":%.2f,\"rejRh\":%lu,\"rejTc\":%lu,\"rejTs\":%lu},"
    "\"id\":{\"valid\":%d,\"gainDC\":%.2f,\"delaySamp\":%d,\"p1\":%.2f,\"p2\":%.2f,\"r2\":%.2f,\"fits\":%lu,\"accepted\":%lu},"
    "\"mpc\":{\"active\":%d,\"u\":%.3f,\"minPredDevC\":%.2f},"
    "\"ml\":{\"eqRaw\":%.4f,\"eqAw\":%.4f,\"arEqRaw\":%.4f,\"disagree\":%.4f},"
    "\"shown\":{\"aw\":%.4f,\"startMix\":%.2f,\"offset\":%.4f,\"zone\":%.2f}}",
    advCfg.kf, advCfg.ifix, advCfg.mpc,
    (int)advKfFresh(), advKf.T(), advKf.Tdot() * 60.0f, advKf.sdT(), advKf.lastNis, advKf.rejectStreak,
    (int)advEkf.init, advEkf.aw(), advEkf.sdAw(), advEkf.awPerMin(), advEkf.sdAwPerMin(), advEkf.delta(), advEkf.sdDelta(),
    (unsigned long)advEkf.nRejRh, (unsigned long)advEkf.nRejTc, (unsigned long)advEkf.nRejTs,
    (int)advId.valid(), advId.gainDC(), advId.best, advId.p1Fit, advId.p2Fit, advId.explained(), (unsigned long)advId.nFits, (unsigned long)advId.nAccepted,
    (int)advMpcOn, advMpcU, advMpc.lastMinY,
    advMlEqRaw, advMlEqAw, predictedEqRaw, advMlDisagree,
    toShown, toStartMix, toOff, toZ);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}
void handleAdvSet() {
  if (!checkAuth()) return;
  if (currentRole != ROLE_ADMIN) { server.send(403, "text/plain", "Admin only"); return; }
  if (server.hasArg("kf"))   advCfg.kf   = server.arg("kf").toInt() != 0;
  if (server.hasArg("ifix")) advCfg.ifix = server.arg("ifix").toInt() != 0;
  if (server.hasArg("mpc"))  advCfg.mpc  = server.arg("mpc").toInt() != 0;
  if (!advCfg.mpc) advMpcOn = false;
  advSaveCfg();
  server.send(200, "text/plain", "OK");
}

void updateAutoHeater() {
  // v17: ฮีตเตอร์ไล่ความชื้น/self-test เป็นฮีตเตอร์ในตัวชิป SHT45 เท่านั้น DHT22/11 ไม่มีฮีตเตอร์ให้สั่งเลย
  // ถ้าตอนนี้กำลังใช้ DHT (หรือยังไม่เจอเซนเซอร์ใดเลย) ฟีเจอร์นี้ไม่มีผลอะไรจริง ให้ข้ามทั้งฟังก์ชันไปเลย
  if (humSensorType != HUMSENS_SHT) return;
  unsigned long now = millis();

  // กำลังฮีตอยู่ (ไม่ว่า auto หรือ manual) — ถ้าเป็น auto ให้เช็คว่าครบเวลา burst แล้วหรือยัง
  if (sensorHeaterOn) {
    if (heaterAutoActive && (now - heaterAutoOnAt >= HEATER_AUTO_BURST_MS)) {
      setSensorHeater(false);
      heaterAutoActive = false;
      heaterLastAutoFireMs = now;
      highRhStartMs = 0; // เริ่มนับสถานะ %RH สูงใหม่หลังฮีตเสร็จ กันไม่ให้ trigger ซ้ำทันทีจากไอน้ำตกค้าง
    }
    return; // ฮีตเตอร์เปิดอยู่ (auto กำลังนับเวลา หรือ manual ที่ auto ไม่ควรยุ่ง) ไม่ต้องเช็คเงื่อนไขอื่นซ้อน
  }

  if (!autoHeaterEnabled) return;
  // ห้ามฮีตระหว่างกำลังวัดค่า aw จริงไม่ว่าจะโหมดไหน (ปกติ/ทำนาย/เปรียบเทียบ) หรือช่วง boot ที่เซนเซอร์ยังไม่นิ่ง
  // (v12: รวมช่วงฮีต/รอเย็นก่อน-หลังวัด ST_SENSOR_COND ที่ระบบคุมฮีตเตอร์เองอยู่แล้วด้วย)
  if (state == ST_MEASURE_AW || state == ST_PREDICT_AW || state == ST_COMPARE_MEASURE || state == ST_SENSOR_COND || state == ST_BOOT_WARMUP) { highRhStartMs = 0; return; }

  if (now - lastAutoHeaterCheckMs < AUTO_HEATER_CHECK_INTERVAL_MS) return;
  lastAutoHeaterCheckMs = now;

  float rh = sht.readHumidity();
  if (isnan(rh)) return;

  bool shouldFire = false;

  // เงื่อนไข 1: %RH สูงเสี่ยงหยดน้ำเกาะต่อเนื่องนานพอ
  if (rh >= HEATER_HIGH_RH_THRESHOLD) {
    if (highRhStartMs == 0) highRhStartMs = now;
    if (now - highRhStartMs >= HEATER_HIGH_RH_SUSTAIN_MS &&
        now - heaterLastAutoFireMs >= HEATER_AUTO_COOLDOWN_MS) {
      shouldFire = true;
    }
  } else {
    highRhStartMs = 0;
  }

  // เงื่อนไข 2: ฮีตป้องกันเซนเซอร์ดริฟท์ระยะยาวตามรอบเวลา เฉพาะตอนเครื่องว่างอยู่ที่เมนูหลัก
  if (!shouldFire && state == ST_MENU_MAIN &&
      now - heaterLastAutoFireMs >= HEATER_MAINT_INTERVAL_MS) {
    shouldFire = true;
  }

  if (shouldFire) {
    setSensorHeater(true);
    heaterAutoActive = true;
    heaterAutoOnAt = now;
    Serial.println("[AutoHeater] fired");
  }
}

// ============================================================================
//  v13: Wi-Fi Access Point ที่เสถียรขึ้น + บันทึกสาเหตุรีเซ็ต/โหมด SAFE START
// ============================================================================
uint8_t wifiChannelInUse = WIFI_FALLBACK_CHANNEL;
uint32_t wifiRestartCount = 0;
unsigned long lastWifiCheckMs = 0;
unsigned long lastWifiRestartMs = 0;

// สแกนคลื่นรอบข้างแล้วเลือกช่อง 1 / 6 / 11 (ช่องเดียวที่ไม่ซ้อนทับกัน) ที่มี AP อื่นรบกวนน้อยสุด
// น้ำหนัก = ความแรงสัญญาณของ AP อื่น (แรง = รบกวนมาก) และช่องข้างเคียง +-2 ก็รบกวนด้วย (ลดตามระยะ)
uint8_t pickWifiChannel() {
  int n = WiFi.scanNetworks(false, false, false, 120);
  esp_task_wdt_reset();
  if (n < 0) { WiFi.scanDelete(); return WIFI_FALLBACK_CHANNEL; }
  int score[15];
  for (int i = 0; i < 15; i++) score[i] = 0;
  for (int i = 0; i < n; i++) {
    int ch = WiFi.channel(i);
    int w = WiFi.RSSI(i) + 100;       // -30 dBm -> 70, -90 dBm -> 10
    if (w < 1) w = 1;
    if (w > 70) w = 70;
    for (int d = -2; d <= 2; d++) {
      int c = ch + d;
      if (c >= 1 && c <= 13) score[c] += w / (1 + abs(d));
    }
  }
  WiFi.scanDelete();
  const uint8_t cand[3] = { 1, 6, 11 };
  uint8_t best = cand[0];
  for (int i = 1; i < 3; i++) if (score[cand[i]] < score[best]) best = cand[i];
  Serial.printf("[WIFI] scan found %d APs -> channel %u (score ch1=%d ch6=%d ch11=%d)\n", n, (unsigned)best, score[1], score[6], score[11]);
  return best;
}

// v-dist: อ่านค่าความแรงสัญญาณ (RSSI, dBm) ของอุปกรณ์ที่เชื่อมต่อ AP ของบอร์ดอยู่ตอนนี้ — คืน false ถ้ายังไม่มีใครเชื่อมต่อ
// ถ้ามีหลายอุปกรณ์เชื่อมต่อพร้อมกัน ใช้ตัวที่สัญญาณแรงสุด (มักเป็นอุปกรณ์ที่กำลังเปิดหน้าเว็บดูอยู่จริง)
bool getClientRssi(int8_t* outRssi) {
  wifi_sta_list_t staList;
  if (esp_wifi_ap_get_sta_list(&staList) != ESP_OK || staList.num <= 0) return false;
  int8_t best = -127;
  for (int i = 0; i < staList.num; i++) {
    if (staList.sta[i].rssi > best) best = staList.sta[i].rssi;
  }
  *outRssi = best;
  return true;
}

// v-dist: แปลง RSSI (dBm) เป็นระยะทางโดยประมาณ (เมตร) ด้วยสูตร log-distance path loss — ดูหมายเหตุที่ค่าคงที่
// WIFI_RSSI_AT_1M / WIFI_PATH_LOSS_N ด้านบนไฟล์ (ใช้เทียบเคียง/อ้างอิงคร่าว ๆ เท่านั้น ไม่ใช่ระยะทางที่วัดแม่นยำ)
float rssiToDistanceM(int8_t rssi) {
  return pow(10.0, ((float)WIFI_RSSI_AT_1M - (float)rssi) / (10.0 * WIFI_PATH_LOSS_N));
}

// เปิด Access Point ด้วยการตั้งค่าครบ: ปิด Wi-Fi ให้สะอาดก่อน -> เลือกช่อง -> โหมด AP ล้วน -> ลดกำลังส่ง -> IP คงที่ -> softAP
bool startWifiAP(bool allowScan) {
  WiFi.persistent(false);   // ไม่เขียนค่า Wi-Fi ลง flash ทุกครั้งที่เปิด AP (ลดการสึกหรอ + กันค่าเก่าค้างชนกัน)
  WiFi.mode(WIFI_OFF);
  delay(50);
  uint8_t ch = WIFI_FALLBACK_CHANNEL;
  if (allowScan && WIFI_AUTO_CHANNEL) {
    WiFi.mode(WIFI_STA);    // การสแกนต้องเปิดฝั่ง STA ชั่วคราว
    WiFi.disconnect();
    delay(50);
    ch = pickWifiChannel();
    WiFi.mode(WIFI_OFF);
    delay(50);
  }
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  WiFi.setTxPower(wifiTxPower);   // ลดกระแสพีคตอนส่งสัญญาณ (ช่วยเรื่องไฟตก/I2C เพี้ยน) — ตั้งค่าไว้ที่ config ด้านบนไฟล์
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  bool ok = WiFi.softAP(ssid, wifiPassword, ch, 0, WIFI_MAX_CLIENTS);
  WiFi.setTxPower(wifiTxPower);   // บางเวอร์ชันของ core รีเซ็ตกำลังส่งตอน softAP() จึงตั้งซ้ำอีกครั้ง
  wifiChannelInUse = ch;
  return ok;
}

// เรียกทุกรอบ loop() — ตรวจทุก WIFI_CHECK_MS ว่า AP ยังทำงานอยู่ ถ้าหลุดจะเปิดใหม่เอง (ไม่เกิน 1 ครั้งต่อ 30 วินาที)
void wifiKeepAlive() {
  unsigned long now = millis();
  if (now - lastWifiCheckMs < WIFI_CHECK_MS) return;
  lastWifiCheckMs = now;
  bool apUp = (((int)WiFi.getMode() & (int)WIFI_AP) != 0) && (WiFi.softAPIP() != IPAddress(0, 0, 0, 0));
  if (apUp) { wifiApFault = false; return; }
  wifiApFault = true;
  if (now - lastWifiRestartMs < 30000UL) return;
  lastWifiRestartMs = now;
  wifiRestartCount++;
  Serial.println("[WIFI] AP is down - restarting");
  wifiApFault = !startWifiAP(false);   // เปิดใหม่แบบไม่สแกน (เร็วและไม่กินกระแสเพิ่ม)
  server.begin();
}

// ---------- บันทึกสาเหตุรีเซ็ต ----------
esp_reset_reason_t lastResetReason = ESP_RST_UNKNOWN;
uint32_t bootBrownoutCount = 0, bootWdtCount = 0, bootPanicCount = 0;
int bootAbnormalStreak = 0;     // รีเซ็ตผิดปกติติดกันกี่ครั้ง (ล้างเมื่อเครื่องทำงานปกติเกิน BOOT_STABLE_CLEAR_MS หรือเสียบไฟใหม่)
bool safeStart = false;         // true = โหมด SAFE START (ลดโหลดไฟเลี้ยงทุกอย่าง)
bool bootStreakCleared = false;

const char* resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWER-ON";
    case ESP_RST_EXT:       return "EXT-RESET";
    case ESP_RST_SW:        return "SOFTWARE";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT-WDT";
    case ESP_RST_TASK_WDT:  return "TASK-WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    default:                return "UNKNOWN";
  }
}

// เรียกครั้งเดียวต้นๆ setup(): เขียน NVS เฉพาะเมื่อเกิดรีเซ็ตผิดปกติ (ไม่เขียนทุกครั้งที่บูต เพื่อไม่ให้ flash สึกเวลาบูตวน)
void loadBootDiagnostics() {
  lastResetReason = esp_reset_reason();
  prefs.begin("awboot", false);
  bootBrownoutCount = prefs.getUInt("brown", 0);
  bootWdtCount = prefs.getUInt("wdt", 0);
  bootPanicCount = prefs.getUInt("panic", 0);
  int streak = prefs.getUChar("streak", 0);
  bool abnormal = (lastResetReason == ESP_RST_BROWNOUT || lastResetReason == ESP_RST_TASK_WDT ||
                   lastResetReason == ESP_RST_INT_WDT || lastResetReason == ESP_RST_WDT || lastResetReason == ESP_RST_PANIC);
  if (abnormal) {
    if (lastResetReason == ESP_RST_BROWNOUT) { bootBrownoutCount++; prefs.putUInt("brown", bootBrownoutCount); }
    else if (lastResetReason == ESP_RST_PANIC) { bootPanicCount++; prefs.putUInt("panic", bootPanicCount); }
    else { bootWdtCount++; prefs.putUInt("wdt", bootWdtCount); }
    if (streak < 250) streak++;
    prefs.putUChar("streak", (uint8_t)streak);
  } else if (lastResetReason == ESP_RST_POWERON && streak != 0) {
    streak = 0;                                    // เสียบไฟใหม่ = เริ่มนับใหม่
    prefs.putUChar("streak", 0);
  }
  prefs.end();
  bootAbnormalStreak = streak;
  safeStart = (streak >= SAFE_START_AFTER_RESETS);
}

// เรียกทุกรอบ loop(): ทำงานปกติครบเวลา = ถือว่าบูตครั้งนี้เสถียร ล้างตัวนับรีเซ็ตติดกัน (SAFE START ของบูตนี้ยังคงอยู่จนกว่าจะรีสตาร์ท)
void bootStabilityService() {
  if (bootStreakCleared || millis() < BOOT_STABLE_CLEAR_MS) return;
  bootStreakCleared = true;
  if (bootAbnormalStreak != 0) {
    prefs.begin("awboot", false);
    prefs.putUChar("streak", 0);
    prefs.end();
    bootAbnormalStreak = 0;
  }
}

// สแกนบัส I2C แล้วพิมพ์ทุกที่อยู่ที่ตอบกลับลง Serial (ช่วยไล่ปัญหาสาย/ที่อยู่)
void scanI2CToSerial() {
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("[I2C] found device at 0x%02X\n", a);
      found++;
    }
  }
  if (!found) Serial.println("[I2C] no devices found - check SDA/SCL/GND/3.3V");
}

void setup() {
  // v13: เรื่องแรกสุด — บังคับขาเทลเทียร์/พัดลมเป็น LOW ทันที (ก่อนโค้ดอื่นทั้งหมด) เพื่อให้โหลดกำลังสูงปิดแน่นอนตั้งแต่ต้นบูต
  // (ก่อนหน้านี้ขานี้ลอยอยู่จนกว่าจะถึง ledcAttach() — ขาลอยอาจทำให้ MOSFET เปิดครึ่งๆ กลางๆ และกินกระแสตอนไฟยังไม่นิ่ง)
  // ควรต่อตัวต้านทาน pull-down ~10k ที่ขา GATE/TRIG ของโมดูลขับเทลเทียร์ด้วยเพื่อให้ปิดแน่นอนแม้ก่อนโค้ดเริ่มทำงาน
  pinMode(PELTIER_PWM_PIN, OUTPUT);
  digitalWrite(PELTIER_PWM_PIN, LOW);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  setLED(false, false, false);

  loadBootDiagnostics();   // v13: รู้ว่าบูตครั้งนี้มาจากอะไร (ไฟตก/watchdog/เสียบไฟ) และเข้าโหมด SAFE START หรือไม่
  if (safeStart) {
    // รีเซ็ตผิดปกติติดกันหลายครั้ง = ไฟเลี้ยงน่าจะไม่พอ -> ลดโหลดทุกอย่างลงให้มากที่สุดเท่าที่ทำได้
    wifiTxPower = WIFI_POWER_8_5dBm;
    peltierBootDelayMs = 30000UL;
    peltierSlewPerS = 40.0f;
  }
  setCpuFrequencyMhz(safeStart ? SAFE_CPU_FREQ_MHZ : CPU_FREQ_MHZ);   // ลดความถี่ CPU = ลดกระแส (ต้องทำก่อน Serial.begin)

  Serial.begin(115200); // ไว้ log เหตุการณ์ฮีตเตอร์อัตโนมัติ (เปิด Serial Monitor เพื่อดูหลักฐานว่าฮีตตอนไหน)
  Serial.printf("[BOOT] reset reason: %s | consecutive abnormal resets: %d | brownouts total: %lu | SAFE START: %s\n",
                resetReasonName(lastResetReason), bootAbnormalStreak, (unsigned long)bootBrownoutCount, safeStart ? "YES" : "no");

  // v-stability: เปิด task watchdog ของ ESP32 เอง — ถ้า loop() ไม่วนกลับมาเรียก esp_task_wdt_reset()
  // ภายใน WDT_TIMEOUT_SEC วินาที (เช่น โค้ดค้างเพราะ I2C แฮงก์) บอร์ดจะรีบูตตัวเองแทนที่จะค้างเงียบ ๆ ตลอดไป
  // v13: ป้อน watchdog ระหว่างขั้นตอนใน setup() ด้วย (เดิมรอจนถึงหน้าจอต้อนรับ ถ้าขั้นตอนก่อนหน้าช้ารวมกันเกิน 10 วิ = รีบูตวน)
  esp_task_wdt_config_t twdtConfig = {
    .timeout_ms = (uint32_t)WDT_TIMEOUT_SEC * 1000, // core 3.x: timeout ระบุเป็น ms ผ่าน struct แทนอาร์กิวเมนต์ตรง ๆ
    .idle_core_mask = 0,
    .trigger_panic = true,                          // panic (reboot) เมื่อ timeout
  };
  esp_task_wdt_init(&twdtConfig);
  esp_task_wdt_add(NULL);                    // เฝ้าดู task หลักที่รัน setup()/loop()
  i2cBusRecover();          // v13: เคลียร์บัสก่อนเริ่มใช้ (ถ้าเพิ่งรีเซ็ตกลางการส่งข้อมูล อุปกรณ์อาจค้างดึง SDA ต่ำอยู่) แล้ว Wire.begin + ตั้ง 50 kHz + timeout
  i2cRecoverCount = 0;
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  ledcAttach(PELTIER_PWM_PIN, PELTIER_PWM_FREQ, PELTIER_PWM_RES); // core 3.x: ผูกขา+ตั้งค่าความถี่/ความละเอียดในคำสั่งเดียว ไม่ต้องระบุช่องเอง
  ledcWrite(PELTIER_PWM_PIN, 0); // เริ่มต้นให้ปิดเทลเทียร์ไว้ก่อนเสมอ (core 3.x: ledcWrite อ้างอิงด้วยขา ไม่ใช่ช่อง)
  esp_task_wdt_reset();

  tft.init();
  tft.setRotation(3);  // หมุนจอ 180 องศาจากเดิม (เดิมคือ 1) ถ้าทิศยังไม่ถูกใจ ลองเปลี่ยนเป็น 0 หรือ 2 ได้
  screenW = tft.width();
  screenH = tft.height();
  graphX = 32;
  graphY = 38; // เผื่อพื้นที่ด้านบนไว้แสดงค่าอุณหภูมิเล็ก ๆ ควบคู่กับค่า aw
  graphW = screenW - graphX - 6;
  if (graphW > MAX_POINTS) graphW = MAX_POINTS; // กันไม่ให้ values[]/tempValues[] ล้น buffer บนจอกว้าง (เคยทำให้กราฟค้าง/พังบางจอ)
  graphH = screenH - graphY - 16;

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("RH sensor init...");
  // v17: ตรวจจับอัตโนมัติว่าตอนนี้ต่อ SHT45 (I2C) หรือ DHT22/11 (GPIO15) อยู่ — เดิมสมมติว่าเป็น SHT เสมอ
  // (สายหลวม/บัดกรีไม่ติด/ที่อยู่ I2C ผิด/ยังไม่ได้เสียบเซนเซอร์เลย จะตั้ง sensorFaultSHT ไว้ตั้งแต่ต้น ให้จอ/เว็บแจ้งเตือนได้ทันที)
  scanI2CToSerial();  // พิมพ์รายชื่ออุปกรณ์ I2C ที่เจอลง Serial Monitor (ใช้ดูว่า SHT35 อยู่ที่ 0x44 หรือ 0x45)
  detectHumiditySensor();
  esp_task_wdt_reset();
  setSensorHeater(false); // เริ่มต้นให้ฮีตเตอร์ในตัวเซนเซอร์ปิดไว้ก่อนเสมอ (มีผลเฉพาะตอนตรวจพบ SHT — DHT ไม่มีฮีตเตอร์ในตัว)
  beginDS18B20();
  // v-stability: เช็คว่าเจออุปกรณ์บนบัส 1-Wire จริงหรือไม่ (สาย DQ หลุด/ไม่ได้ต่อ R pull-up จะเจอ 0 ตัว)
  sensorFaultDS18B20 = (ds18b20.getDeviceCount() == 0);
  if (sensorFaultDS18B20) Serial.println("[FAULT] DS18B20 not found on 1-Wire bus - check wiring/pull-up resistor");
  // Phase 1+2 (calibration roadmap): จับค่าความชื้น/อุณหภูมิ "ห้องเปล่า" ตอนนี้ทันที ก่อนฮีตเตอร์เซนเซอร์
  // เคยถูกเปิดแม้แต่ครั้งเดียวในรอบบูตนี้ - ต้องทำก่อนการวัดจริงใด ๆ เสมอ (DS18B20 begin() ไม่เกี่ยวกับฮีตเตอร์ SHT
  // จึงเรียกก่อนได้ - แค่ต้องรอให้แปลงค่าเสร็จก่อนอ่าน ด้วย waitForDS18B20ConversionOnce() ด้านล่าง)
  loadAmbientBaseline();
  loadLearnedOffset();   // Phase 2: โหลดค่า offset ที่เรียนรู้สะสมไว้ ก่อน capture ตัวอย่างใหม่ของบูตนี้
  if (humSensorType != HUMSENS_NONE) captureAmbientBaseline();
  // v6.3: applyCal() ใช้ตารางจุดคาลิเบรตแบบ piecewise-linear (ดูส่วน CalPoint ด้านบนไฟล์)
  // v-accuracy: จุดคาลิเบรตแก้ไขได้จากหน้าเว็บแล้ว (ไม่ต้องแก้ซอร์สโค้ดอีกต่อไป) จึงต้องโหลดค่าที่เคยบันทึกไว้จาก
  // NVS กลับมาใช้ตอนบูตทุกครั้งผ่าน loadCalPoints() — ถ้าไม่เคยบันทึกไว้เลยจะ fallback ไปใช้ค่าโรงงานอัตโนมัติ
  loadCalPoints();
  loadCalSurface();   // v27: โหลดพื้นผิวคาลิเบรต 2 มิติ (ถ้าไม่เคยบันทึก = ตัวคูณ 1.0 ทั้งหมด)
  // v20: ถ้าเคย Auto-Tune PID ของ "ห้องนี้" (อุณหภูมิห้องตอนบูต ปัดเป็นองศาเต็ม) ไว้ก่อนหน้าแล้ว โหลดเกนที่เคยจูน
  // ไว้ของห้องนั้นกลับมาใช้ทันที — ไม่งั้นใช้ค่าเริ่มต้น (pid_Kp/Ki/Kd ด้านบนไฟล์) ไปก่อนจนกว่าจะสั่งจูนจากเว็บ
  loadPidProfileForEnv(ambientTempCatBoot);
  loadClockFromNVS(); // v-pro: โหลดเวลา/ชื่อผู้ปฏิบัติงานล่าสุดที่เคยซิงก์ไว้ (ถ้ามี) กลับมาใช้ก่อนซิงก์ใหม่ผ่านเว็บ
  esp_task_wdt_reset();
  delay(150);         // v13: เว้นจังหวะให้ไฟเลี้ยงนิ่งหลังจอ/เซนเซอร์ถูกเปิด ก่อนสั่งวิทยุ Wi-Fi (โหลดที่กินกระแสพีคสูงสุด)

  // ปล่อย Wi-Fi และเปิด Web Server
  // v-stability: เช็คผลลัพธ์จริงของ softAP() — ถ้าเปิด AP ไม่สำเร็จ (เช่น ชนกับการตั้งค่า Wi-Fi อื่นค้างอยู่)
  // จะตั้ง wifiApFault ไว้และลองซ้ำอีกครั้งหนึ่ง แทนที่จะปล่อยให้เว็บแดชบอร์ดใช้งานไม่ได้แบบไม่มีใครรู้
  wifiApFault = !startWifiAP(!safeStart);          // v13: ตั้งค่า AP ครบชุด (ช่อง/กำลังส่ง/IP/จำนวนผู้ใช้) — ดู startWifiAP()
  esp_task_wdt_reset();
  if (wifiApFault) {
    Serial.println("[FAULT] WiFi AP failed on first try - retrying once");
    delay(300);
    wifiApFault = !startWifiAP(false);
    if (wifiApFault) Serial.println("[FAULT] WiFi AP failed again - web dashboard will be unreachable (will keep retrying in loop)");
  }
  peltierStartAllowedMs = millis() + peltierBootDelayMs;   // v13: เทลเทียร์/พัดลมเริ่มได้หลังจาก Wi-Fi ขึ้นแล้วช่วงหนึ่ง (ไฟเลี้ยงนิ่งก่อน)
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/info", HTTP_GET, handleDeviceInfo);  // v11: เมทาดาทาอุปกรณ์/คาลิเบรต สำหรับแนบหัวไฟล์ export
  server.on("/calpoints", HTTP_GET, handleCalGet);   // v-accuracy: อ่านจุดคาลิเบรตปัจจุบัน+ค่าโรงงาน
  server.on("/calset", HTTP_GET, handleCalSet);      // v-accuracy: บันทึกจุดคาลิเบรตใหม่ (ใช้งานทันที ไม่ต้องรีบูต)
  server.on("/calreset", HTTP_GET, handleCalReset);  // v-accuracy: รีเซ็ตจุดคาลิเบรตกลับเป็นค่าโรงงาน
  server.on("/calquick", HTTP_GET, handleCalQuick);  // v22: คาลิเบรตแบบเร็วจากเครื่องอ้างอิงภายนอก (ผสมกับตารางเดิม)
  server.on("/pidset", HTTP_GET, handlePidSet);       // v-pid-tune: ปรับเกน PID เทลเทียร์สดจากเว็บ
  server.on("/pidautotune", HTTP_GET, handlePidAutoTuneStart);   // v20: เริ่ม/ยกเลิกจูน PID อัตโนมัติ
  server.on("/pidautotune/status", HTTP_GET, handlePidAutoTuneStatus);
  server.on("/adv/status", HTTP_GET, handleAdvStatus);   // v29
  server.on("/adv/set", HTTP_GET, handleAdvSet);         // v29
  advLoadCfg(); // v20: สถานะสดของการจูนอัตโนมัติ
  server.on("/clocksync", HTTP_GET, handleClockSync); // v-pro: เบราว์เซอร์ซิงก์เวลาจริงให้เครื่อง (audit trail)
  server.on("/setop", HTTP_GET, handleSetOperator);   // v-pro: ตั้งชื่อย่อผู้ปฏิบัติงานปัจจุบัน (audit trail)
  // v-web-ctrl: ล็อกอิน + สั่งวัดจากเว็บ + โหมดคาลิเบตอัตโนมัติของแอดมิน
  server.on("/whoami", HTTP_GET, handleWhoAmI);
  server.on("/cmd/measure", HTTP_GET, handleCmdMeasure);
  server.on("/cmd/cancel", HTTP_GET, handleCmdCancel);
  server.on("/admin/calmode/start", HTTP_GET, handleAdminCalStart);
  server.on("/admin/calmode/status", HTTP_GET, handleAdminCalStatus);
  server.on("/admin/calmode/cancel", HTTP_GET, handleAdminCalCancel);
  server.on("/admin/calsurface/commit", HTTP_GET, handleAdminSurfaceCommit);   // v27: บันทึกพื้นผิว 2 มิติจากผลคาลิเบตอัตโนมัติที่เสร็จแล้ว
  server.on("/admin/calsurface/status", HTTP_GET, handleAdminSurfaceStatus);
  server.on("/admin/calsurface/clear", HTTP_GET, handleAdminSurfaceClear);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  dashboardStartTime = millis();
  lastWifiCheckMs = millis();

  showWelcomeScreen();
  updateDS18B20();
}

void loop() {
  esp_task_wdt_reset(); // v-stability: ป้อน watchdog ทุกรอบ ยืนยันว่า loop() ยังวิ่งอยู่ปกติ ไม่ได้ค้าง
  server.handleClient(); // จัดการ Request จากเว็บ
  lcd.service();         // v13: ซ่อมจอ LCD ที่เพี้ยน (รีเฟรช/init ใหม่เป็นระยะ + เช็ค ACK)
  wifiKeepAlive();       // v13: ตรวจว่า Wi-Fi AP ยังทำงาน ถ้าหลุดเปิดใหม่เอง
  bootStabilityService();// v13: ทำงานปกติครบเวลา -> ล้างตัวนับรีเซ็ตผิดปกติติดกัน
  updateColdRoomFlag();  // v14: เตือนเมื่อห้องแอร์เย็นกว่าเป้าหมายจนเทลเทียร์คุมไม่ได้

  ButtonEvents e = pollButtons();
  updateDS18B20();
  maybeRedetectHumiditySensor(); // v17: ถ้ายังไม่เจอเซนเซอร์เลยตอนบูต ลองตรวจซ้ำเป็นระยะตอนเครื่องว่าง เผื่อเพิ่งเสียบเซนเซอร์เข้าไปทีหลัง
  updateAutoCalMode();   // v-web-ctrl: ปรับ TARGET_TEMP_C ตามรอบคาลิเบตอัตโนมัติ (ถ้าแอดมินกำลังรันอยู่) ก่อนควบคุมเทลเทียร์
  updateAutoCalLED();    // v-led: ไฟสถานะคาลิเบตจากเว็บ (ไม่ทำอะไรถ้าไม่ได้กำลังรันอยู่) — ต้องมาก่อน case เมนูด้านล่าง
  updatePeltierControl();
  updateAutoHeater();

  switch (state) {
    case ST_BOOT_WARMUP:
      {
        if (!bootStaticDrawn) drawBootWarmupStatic();
        unsigned long nowUI = millis();
        if (nowUI - lastBootUIUpdateMs >= BOOT_UI_UPDATE_MS) {
          lastBootUIUpdateMs = nowUI;
          drawBootWarmupDynamic();
        }
        // v12: เปลี่ยนจากไฟกระพริบเป็นสีไฟสถานะนิ่งแบบโรงงานจริง (Andon light) ให้อ่านสถานะได้ทันที
        //   เขียวนิ่ง  = พร้อมทำงาน (ถึงอุณหภูมิเป้าหมาย TARGET_TEMP_C แล้ว)
        //   เหลืองนิ่ง = กำลังทำงาน (เทลเทียร์กำลังลดอุณหภูมิลงสู่เป้าหมาย 25°C)
        //   แดงนิ่ง   = ผิดปกติ/หยุด (ห้องร้อนเกินกำลังเทลเทียร์ ไล่อุณหภูมิลงไม่ถึงเป้าหมาย)
        //   ม่วงกระพริบเร็ว = fault ของเซนเซอร์/Wi-Fi AP (v22) — สำคัญกว่าทุกสถานะข้างบน ต้องแก้ก่อนใช้งานเครื่องต่อ
        if (systemHasFault()) blinkLED(true, false, true, 300);
        else if (warmupDone) setLED(false, true, false);
        else if (peltierStuckHot) setLED(true, false, false);
        else setLED(true, true, false);

        if (warmupDone || e.select) {
          setLED(false, false, false);
          lcd.clear();
          state = ST_MENU_MAIN;
          selIndex = 0;
          prevDrawnState = (AppState)-1;
        }
        break;
      }
    case ST_MENU_MAIN:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต (updateAutoCalLED() ตั้งไว้แล้วด้านบน) ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(false);
        if (e.up || e.down) selIndex = stepSelIndex(selIndex, 4, e.up, e.down);
        if (e.select) {
          if (selIndex == 0) {
            state = ST_MENU_AW;
            selIndex = 0;
          } else if (selIndex == 1) {
            loadRecordingIndex();
            state = ST_MENU_RECORD;
            selIndex = 0;
          } else if (selIndex == 2) {
            state = ST_WIFI_INFO;
            prevDrawnState = (AppState)-1;
          } else {
            state = ST_SYS_HEALTH;
            prevDrawnState = (AppState)-1;
          }
        }
        break;
      }
    case ST_MENU_AW:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต (updateAutoCalLED() ตั้งไว้แล้วด้านบน) ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(false);
        if (e.up || e.down) selIndex = stepSelIndex(selIndex, 4, e.up, e.down);
        if (e.select) {
          const char* blockReason = "";
          if (selIndex <= 2 && !canStartMeasurement(&blockReason)) {
            showStartBlockedWarning(blockReason);
          } else if (selIndex == 0) {
            enterMeasureAW();
          } else if (selIndex == 1) {
            enterPredictAW();
          } else if (selIndex == 2) {
            enterCompareAW();
          } else {
            state = ST_MENU_MAIN;
            selIndex = 0;
          }
        }
        break;
      }
    case ST_MEASURE_AW:
      {
        runMeasureAWTick();
        if (state == ST_MEASURE_AW && savePromptActive) {
          // v12: ป็อปอัป "Save?" ค้างอยู่ทับกราฟ (กราฟยังวิ่งต่อ) — UP/DN สลับตัวเลือก, ค้าง 1 ปุ่ม = ยืนยัน
          if (e.up || e.down) {
            promptSel = stepSelIndex(promptSel, 2, e.up, e.down);
            drawPromptPopupTFT();
            drawPromptLCD();
          }
          if (e.select) handleMeasurePromptChoice();
        }
        if (state == ST_MEASURE_AW && e.exit) {
          // กดออก = เลิกวัดโดยไม่บันทึก (เหมือนเดิม) แต่ v12 ให้ฮีตไล่ไอน้ำหลังวัดก่อนกลับเมนู (ข้ามได้ด้วยการกดค้าง)
          savePromptActive = false;
          startSensorConditioning(false, COND_NEXT_MENU_AW, 0);
        }
        break;
      }
    case ST_PREDICT_AW:
      {
        runPredictAWTick();
        if (e.exit) {
          startSensorConditioning(false, COND_NEXT_MENU_AW, 1);
        }
        break;
      }
    case ST_COMPARE_MEASURE:
      {
        runCompareTick();
        if (state == ST_COMPARE_MEASURE && savePromptActive) {
          // v12: ตัวอย่าง A นิ่ง -> ป็อปอัป "Measure B next?"  /  ตัวอย่าง B นิ่ง -> ป็อปอัป "Save A and B?"
          // (กราฟของตัวอย่างนั้นยังวิ่งต่อจนกว่าจะกดเลือก)
          if (e.up || e.down) {
            promptSel = stepSelIndex(promptSel, 2, e.up, e.down);
            drawPromptPopupTFT();
            drawPromptLCD();
          }
          if (e.select) handleComparePromptChoice();
        }
        if (state == ST_COMPARE_MEASURE && e.exit) {
          savePromptActive = false;
          startSensorConditioning(false, COND_NEXT_MENU_AW, 2);
        }
        break;
      }
    case ST_COMPARE_RESULT:
      {
        // v12: หน้าสรุปนี้แสดงหลังตอบป็อปอัปของตัวอย่าง B แล้ว (บันทึกไปแล้วหรือไม่ ดู compareSaved) กดอะไรก็ได้เพื่อออก
        if (e.select || e.exit) {
          startSensorConditioning(false, COND_NEXT_MENU_AW, 2);
        }
        break;
      }
    case ST_SENSOR_COND:
      {
        runSensorCondTick(e);
        break;
      }
    case ST_MENU_RECORD:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่ (เดินดูบันทึกระหว่างรอคาลิเบตได้)
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(true);
        if (recCount == 0) {
          if (e.select || e.exit) {
            state = ST_MENU_MAIN;
            selIndex = 1;
          }
        } else {
          if (e.up || e.down) selIndex = stepSelIndex(selIndex, recCount + 1, e.up, e.down);
          if (e.select) {
            if (selIndex == recCount) {
              state = ST_MENU_MAIN;
              selIndex = 1;
            } else {
              recItemIndex = selIndex;
              snprintf(recItemTitle, sizeof(recItemTitle), "#%lu aw:%.3f", (unsigned long)recList[selIndex].id, recList[selIndex].aw);
              state = ST_RECORD_ITEM_MENU;
              selIndex = 0;
              prevDrawnState = (AppState)-1;
            }
          }
          if (e.exit) {
            state = ST_MENU_MAIN;
            selIndex = 1;
          }
        }
        break;
      }
    case ST_RECORD_ITEM_MENU:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(true);
        if (e.up || e.down) selIndex = stepSelIndex(selIndex, 3, e.up, e.down);
        if (e.select) {
          if (selIndex == 0) {
            loadRecordingSnapshot(recSlotOf[recItemIndex]);
            state = ST_RECORD_VIEW;
            prevDrawnState = (AppState)-1;
          } else if (selIndex == 1) {
            deleteRecording(recItemIndex);
            state = ST_MENU_RECORD;
            selIndex = 0;
            prevDrawnState = (AppState)-1;
          } else {
            state = ST_MENU_RECORD;
            selIndex = recItemIndex;
            prevDrawnState = (AppState)-1;
          }
        }
        if (e.exit) {
          state = ST_MENU_RECORD;
          selIndex = recItemIndex;
          prevDrawnState = (AppState)-1;
        }
        break;
      }
    case ST_RECORD_VIEW:
      {
        if (e.select || e.exit) {
          state = ST_MENU_RECORD;
          selIndex = recItemIndex;
          prevDrawnState = (AppState)-1;
        }
        break;
      }
    case ST_WIFI_INFO:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(false);
        if (e.select || e.exit) {
          state = ST_MENU_MAIN;
          selIndex = 2;
          prevDrawnState = (AppState)-1;
        }
        break;
      }
    case ST_SYS_HEALTH:
      {
        // v-led: อย่าทับไฟสถานะคาลิเบต ถ้าแอดมินกำลังรันคาลิเบตจากเว็บอยู่
        if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) idleStatusLED(false);
        if (e.select || e.exit) {
          state = ST_MENU_MAIN;
          selIndex = 3;
          prevDrawnState = (AppState)-1;
        }
        break;
      }
  }

  if (state != prevDrawnState || selIndex != prevDrawnSel) {
    switch (state) {
      case ST_MENU_MAIN: drawMenuScreen("MAIN MENU", mainItems, mainIcons, 4, selIndex);
        break;
      case ST_MENU_AW: drawMenuScreen("AW MENU", awItems, awIcons, 4, selIndex); break;
      case ST_COMPARE_RESULT: drawCompareResultScreen(); break;
      case ST_MENU_RECORD: drawRecordListScreen(); break;
      case ST_RECORD_ITEM_MENU: drawMenuScreen(recItemTitle, recItemItems, recItemIcons, 3, selIndex); break;
      case ST_RECORD_VIEW: drawRecordViewScreen(); break;
      case ST_WIFI_INFO: drawWifiInfoScreen(); break;
      case ST_SYS_HEALTH: drawSysHealthScreen(); break;
      default: break;
    }
    prevDrawnState = state;
    prevDrawnSel = selIndex;
  }

  // ทำให้มาสคอตหน้าเมนูหลักกระพริบตาเป็นระยะ ๆ (เรียกทุกรอบลูป แต่วาดจริงเฉพาะตอนถึงจังหวะกระพริบ)
  if (state == ST_MENU_MAIN) updateMascotIdleBlink();

  // v-acal-graph: ระหว่างโหมดคาลิเบตอัตโนมัติของแอดมิน (เครื่องว่างอยู่ที่เมนู) โชว์กราฟ aw สดของรอบปัจจุบันบนจอเครื่องด้วย
  // เหมือนกับที่โชว์บนเว็บ ให้เห็นตรงกันทั้งสองที่ — วาดทับมุมจอเป็นระยะ ไม่รบกวนเมนูหลัก
  if ((state == ST_MENU_MAIN || state == ST_MENU_AW) && (autoCalPhase == ACAL_COOLING || autoCalPhase == ACAL_MEASURING)) {
    updateAutoCalBoardGraph();
    updateAutoCalLCD();  // v-pro: LCD ก็อิงตามเว็บเหมือนกันระหว่างคาลิเบต ไม่ใช่แค่จอ TFT
  }
}

// v-stability: กันผู้ใช้เริ่มวัดค่าทั้งที่เซนเซอร์หลุด หรือห้องร้อนเกินกำลังเทลเทียร์จนอุณหภูมิยังไม่นิ่ง
// (peltierStuckHot) — เดิมกดเริ่มวัดได้เสมอแม้เงื่อนไขพวกนี้จะทำให้ผลวัดไม่น่าเชื่อถือ ผู้ใช้ไม่มีทางรู้เลย
bool canStartMeasurement(const char** reasonOut) {
  if (sensorFaultSHT) { *reasonOut = "SHT SENSOR ERROR"; return false; }
  if (sensorFaultDS18B20) { *reasonOut = "TEMP SENSOR ERROR"; return false; }
  if (peltierStuckHot) { *reasonOut = "ROOM TOO HOT"; return false; }
  if (roomEnvironmentChanged) { *reasonOut = "ROOM RH CHANGED"; return false; }
  return true;
}

// แจ้งเตือนสั้น ๆ บนจอ+ไฟสถานะตอนผู้ใช้พยายามเริ่มวัดทั้งที่ canStartMeasurement() ไม่ผ่าน แล้วกลับไปที่เมนูเดิม
void showStartBlockedWarning(const char* reason) {
  setLED(true, false, false);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CANNOT START:");
  lcd.setCursor(0, 1);
  String l = String(reason);
  while (l.length() < 16) l += ' ';
  lcd.print(l.substring(0, 16));
  tft.fillScreen(COL_BG);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(COL_WARN, COL_BG);
  tft.drawString("CANNOT START MEASURE", screenW / 2, screenH / 2 - 14, 2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(reason, screenW / 2, screenH / 2 + 12, 2);
  tft.setTextDatum(TL_DATUM);
  delay(1600);
  setLED(false, false, false);
  prevDrawnState = (AppState)-1;  // บังคับให้วาดเมนูเดิมใหม่หลังเคลียร์จอ
}

// v12: enterXxx() = จุดเข้าจากเมนู -> ฮีต/รอเย็นเซนเซอร์ก่อน แล้วค่อยเรียก beginXxx() (ตัวที่เริ่มวัดจริง ซึ่งเป็นโค้ดเดิม)
void enterMeasureAW() {
  startSensorConditioning(true, COND_NEXT_MEASURE, 0);
}

void beginMeasureAW() {
  state = ST_MEASURE_AW;
  selIndex = 0;
  numPoints = 0;
  resetGraphFilter();
  iconVisible = false;
  savePromptActive = false;
  resetStabilityWindow();   // measureStartMs เริ่มนับตรงนี้ = หลังฮีต/รอเย็นเสร็จแล้ว (เวลาฮีตไม่นับเป็นเวลาวัด)
  tft.fillScreen(COL_BG);
  drawStaticUI();
  lcd.clear();
  lastUpdate = 0;
}

void runMeasureAWTick() {
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();

  float raw;
  currentAw = readAwAndRaw(raw);
  pushValue(currentAw, currentTempC);
  updateStabilityWindow(raw, currentTempC);

  // v12: กราฟนิ่งครั้งแรก -> เปิดป็อปอัป "Save?" (latch: ค้างไว้จนกว่าผู้ใช้จะเลือก แม้ค่าจะแกว่งออกไปทีหลัง)
  // แต่ไม่หยุดวัด — ทุกอย่างด้านล่างยังทำงานต่อทุกรอบเหมือนเดิม
  if (measureStable && !savePromptActive) {
    savePromptActive = true;
    promptSel = 0;
    refreshPromptValues();
    triggerIcon(promptAw, iconForCategory[getFoodCategoryIndex(promptAw)]);  // ไอคอนหมวดอาหารเด้งที่ปลายกราฟ 4 วินาที เหมือนตอนหน้า "วัดเสร็จ" เดิม
  }
  if (savePromptActive) refreshPromptValues();

  // v16: สีเดียวกันทุกโหมด อ่านจากตัวตรวจนิ่งกลาง (stabPhase: 0 แดง=ยังเคลื่อนที่ / 1 เหลือง=คงที่ 20 วิ / 2 เขียว=ล็อกได้)
  int holdPhase = stabPhase;
  // v22: เหลืองอยู่แต่ช่วงกว้างหน้าต่าง 60 วิแคบจนเกือบผ่านเกณฑ์ล็อกแล้ว (แค่รอเวลา/ความชัน) = ใกล้นิ่งมาก -> เขียวกระพริบ
  bool nearStable = (holdPhase == 1) && !isnan(stabRangeAw) && (stabRangeAw <= STAB_TOL * 1.3f);

  updateMeasureLEDs(holdPhase, nearStable);
  drawGraph(holdPhase);
  drawCurrentValue(currentAw, holdPhase);
  drawCurrentTemp(currentTempC);
  int idx = getFoodCategoryIndex(currentAw);
  drawElapsedTimeTFT(measureStartMs);
  if (savePromptActive) {
    if (iconVisible && millis() - iconShownAt < iconShowDuration) drawActiveIcon(iconX, iconY, millis() - iconShownAt);
    drawPromptPopupTFT();   // วาดทับหลังกราฟ (drawGraph ล้างพื้นที่กราฟใหม่ทุกรอบ)
    drawPromptLCD();
  } else {
    updateLCD(currentAw, holdPhase, idx);
  }
}

// v13: เปลี่ยนจากไฟกระพริบเป็นสีนิ่งแบบเดียวกับตอนเปิดเครื่อง (Andon) ให้เป็นระบบไฟสถานะเดียวกันทั้งเครื่อง
//   เขียวนิ่ง  = นิ่งแล้ว ล็อกค่าได้ (holdPhase 2)
//   เหลืองนิ่ง = ค่าคงที่มาแล้ว >= 20 วิ (holdPhase 1) กำลังรอให้ครบหน้าต่าง 60 วิ + ความชันต่ำ  [v16]
//   แดงนิ่ง   = ยังไม่นิ่ง กำลังปรับตัว/ไล่ค่าอยู่ (holdPhase 0)
// v22: เพิ่ม nearStable — เมื่อ holdPhase==1 (เหลือง) แต่ช่วงกว้างของหน้าต่าง 60 วิล่าสุดแคบจนเกือบผ่านเกณฑ์ล็อกแล้ว
// (แค่รอเวลา/ความชันให้ครบ) ใช้ "เขียวกระพริบเร็ว" แทน เพราะไฟ RGB ดิจิทัลล้วนทำสีส้มจริงไม่ได้ (ไม่มี PWM ผสมสี)
void updateMeasureLEDs(int holdPhase, bool nearStable) {
  if (holdPhase == 2) setLED(false, true, false);
  else if (holdPhase == 1) {
    if (nearStable) blinkLED(false, true, false, 200);  // เขียวกระพริบเร็ว = ใกล้นิ่งมากแล้ว
    else setLED(true, true, false);
  } else setLED(true, false, false);
}

// v-led: ตัดสิน "ความนิ่ง" ของค่าที่อ่านได้ระหว่าง ACAL_MEASURING โดยดูช่วงกว้าง (max-min) ของกราฟสด
// autoCalGraphBuf (สุ่มค่าทุก ~1 วิ ความจุ AUTOCAL_GRAPH_CAP=520 ตัว เผื่อไว้เกิน STAB_SLOPE_WINDOW_MS/1000 วิ
// ที่หน้าตรวจวัดปกติใช้ตัดสินความนิ่งพอดี — v23) ใช้เกณฑ์ความกว้างเดียวกับ STAB_RANGE_TOL/STAB_NEAR_MULT ของหน้าตรวจวัด
// ปกติ เพื่อให้ "สีเดียวกัน = ความหมายเดียวกัน" ไม่ว่าจะกำลังตรวจวัดปกติหรือกำลังคาลิเบตอยู่ก็ตาม — จงใจใช้ตัวแปร
// ของตัวเอง (ไม่แตะ stabBuf/measureStable ของหน้าตรวจวัดปกติ) เพราะสองโหมดนี้ไม่ควรวิ่งพร้อมกันอยู่แล้ว แยกไว้ให้
// ชัวร์ว่าจะไม่แย่งสถานะกันถ้ามีทางเข้าที่ไม่คาดคิด
//   คืนค่า 0 = ยังไม่นิ่ง (ค่ายังไหลอยู่ หรือยังเก็บตัวอย่างไม่ครบ 5 นาที)
//         1 = ค่าคงที่มาระยะหนึ่งแล้ว (ใกล้นิ่ง) แต่ยังไม่แน่นพอ
//         2 = นิ่งสนิทแล้วตลอดหน้าต่าง 5 นาทีล่าสุด
float acalRangeAw = NAN;  // v22: ช่วงกว้าง aw ของหน้าต่าง 60 วิล่าสุด (เก็บไว้ทำ "เขียวกระพริบ = ใกล้นิ่งมาก" เหมือนโหมดวัดปกติ)
int computeAutoCalHoldPhase() {
  // v16: ใช้ตัวตัดสินตัวเดียวกับโหมดวัดปกติ (stabClassifyBlocks) — autoCalGraphBuf สุ่ม ~1 ตัวอย่าง/วิ จึงรวมทีละ 5 ตัวอย่างเป็น 1 บล็อก 5 วิ
  // เกณฑ์/สีจึงเหมือนกันเป๊ะ (เดิมใช้ STAB_RANGE_TOL 0.008 หลวมกว่าโหมดวัด 10 เท่า -> ไฟเขียวคนละความหมาย)
  const int spb = (int)(STAB_BLOCK_MS / 1000UL);               // ตัวอย่างต่อบล็อก
  if (autoCalGraphCount < spb) { acalPrevHoldPhase = 0; acalPwmSteadyWin = false; acalRangeAw = NAN; return 0; }
  int nBlk = autoCalGraphCount / spb;
  if (nBlk > STAB_BLK_CAP) nBlk = STAB_BLK_CAP;
  int start = autoCalGraphCount - nBlk * spb;
  float blk[STAB_BLK_CAP];
  float pwmBlk[STAB_BLK_CAP];   // v18: PWM (%) เฉลี่ยต่อบล็อก คู่กับ blk[]
  for (int b = 0; b < nBlk; b++) {
    float s = 0, sp = 0;
    for (int k = 0; k < spb; k++) { s += autoCalGraphBuf[start + b * spb + k]; sp += autoCalPwmBuf[start + b * spb + k]; }
    blk[b] = s / spb;
    pwmBlk[b] = sp / spb;
  }
  bool minAgeOk = autoCalGraphCount >= (int)(STAB_SLOPE_WINDOW_MS / 1000UL);
  int ph = stabClassifyBlocks(blk, nBlk, minAgeOk, acalPrevHoldPhase == 2, pwmBlk, &acalRangeAw, NULL);  // v22: ดึง range ออกมาด้วย (เดิมส่ง NULL)
  acalPwmSteadyWin = stabPwmSteady(pwmBlk, nBlk, STAB_WIN_BLKS);   // v18: ใช้ผ่อนเกณฑ์อุณหภูมิตอนตัดสินจบรอบ (updateAutoCalMode)
  acalPrevHoldPhase = ph;
  return ph;
}

// v-led: ไฟสถานะ (Andon) ระหว่างคาลิเบตอัตโนมัติที่สั่งเริ่มจากเว็บ — เรียกทุกรอบ loop() ต่อจาก updateAutoCalMode()
// ตามคำขอผู้ใช้ ("ไฟ LED บ่งบอกสถานะของทุกอย่างในฟังก์ชันตรวจ ไม่ว่าวัดจากเครื่องหรือเว็บ") ก่อนหน้านี้ช่วงคาลิเบตจาก
// เว็บไม่มีไฟสถานะเลย (จอเครื่องแค่โชว์กราฟเฉย ๆ) ใช้ธรรมเนียมสีเดียวกับหน้าตรวจวัดปกติ (updateMeasureLEDs) เป๊ะ
// เพื่อไม่ให้ผู้ใช้ต้องจำความหมายสีคนละชุดสำหรับสองโหมด:
//   แดงนิ่ง  = ACAL_COOLING (กำลังไล่อุณหภูมิเข้าเป้าหมาย) หรือ ACAL_MEASURING ที่ค่ายังไม่นิ่ง (holdPhase 0)
//              หรือห้องร้อนเกินกำลังเทลเทียร์ (peltierStuckHot) ระหว่างไล่อุณหภูมิ
//   เหลืองนิ่ง = ACAL_MEASURING ที่ค่าคงที่มาระยะหนึ่งแล้ว ใกล้นิ่ง (holdPhase 1)
//   เขียวนิ่ง  = ACAL_MEASURING ที่ค่านิ่งสนิทแล้วตลอดหน้าต่าง 5 นาทีล่าสุด (holdPhase 2)
// หมายเหตุ: state ของเครื่องยังเป็น ST_MENU_MAIN/ST_MENU_AW ตามเดิมระหว่างคาลิเบต (ดู updateAutoCalBoardGraph())
// ดังนั้นต้องกันไม่ให้ case เมนูเหล่านั้นใน loop() สั่ง setLED(off) ทับไฟชุดนี้ทุกรอบ (ดูจุดที่แก้ใน loop())
void updateAutoCalLED() {
  if (autoCalPhase == ACAL_IDLE || autoCalPhase == ACAL_DONE) return;
  // v22: fault ของเซนเซอร์/Wi-Fi AP สำคัญกว่าสถานะคาลิเบตปกติ — ม่วงกระพริบชนะทุกกรณีระหว่างคาลิเบตอัตโนมัติด้วยเช่นกัน
  if (systemHasFault()) { blinkLED(true, false, true, 300); return; }
  if (autoCalPhase == ACAL_COOLING) {
    if (peltierStuckHot) setLED(true, false, false);   // แดง: ห้องร้อนเกินกำลังเทลเทียร์ ไล่อุณหภูมิลงไม่ถึงเป้าหมาย
    else setLED(true, true, false);                    // เหลือง: กำลังไล่อุณหภูมิเข้าเป้าหมายตามปกติ (เหมือนตอนบูตเครื่อง)
    return;
  }
  int ph = computeAutoCalHoldPhase();                   // ACAL_MEASURING: ไฟชุดเดียวกับหน้าตรวจวัดปกติเป๊ะ
  bool nearStable = (ph == 1) && !isnan(acalRangeAw) && (acalRangeAw <= STAB_TOL * 1.3f);  // v22
  updateMeasureLEDs(ph, nearStable);
}

// เข้าสู่โหมดทำนายค่า aw สมดุลล่วงหน้า (เมนู "1.2 Predict" แยกต่างหากจากการวัดปกติ "1.1 Start")
void enterPredictAW() {
  startSensorConditioning(true, COND_NEXT_PREDICT, 1);  // v12: ฮีต/รอเย็นก่อน (ช่วงรอเย็นจะได้ไม่ถูกนับเป็นเส้นโค้งที่ใช้ทำนาย)
}

void beginPredictAW() {
  state = ST_PREDICT_AW;
  numPoints = 0;
  resetGraphFilter();
  iconVisible = false;
  predN = 0;
  predBlockSum = 0;
  predBlockCnt = 0;
  predBlockStartMs = millis();
  predStartMs = millis();
  predClear();
  predInvalidStreak = 0;
  predLastUpdate = 0;
  predGraphReset();
  tft.fillScreen(COL_BG);
  drawStaticUI();
  drawPredictLegend();
  lcd.clear();
}

// ---------- ฟิตความสัมพันธ์ y[n+1] = a + k*y[n] ด้วยกำลังสองน้อยสุดถ่วงน้ำหนักตามความใหม่ (weighted AR(1)) ----------
// v15: เดิมเป็น least squares ธรรมดา (ทุกจุดน้ำหนักเท่ากัน) เปลี่ยนเป็นถ่วงน้ำหนักด้วย PRED_RECENCY_DECAY
// ต่อคู่ (y[i], y[i+1]) ให้คู่ที่ใหม่กว่า (i ใกล้ปลายหน้าต่างมากกว่า) มีน้ำหนักมากกว่าคู่เก่า — เหตุผล: ค่าคงที่เวลา
// tau ที่แท้จริงของตัวอย่างจริงอาจไม่คงที่เป๊ะตลอดทั้งเส้นโค้ง (ช่วงต้นอาจมีสัญญาณตกค้างจากฮีต/รอเย็น) จุดใหม่
// จึงเป็นตัวแทนของพลวัตใกล้จุดสมดุลที่กำลังจะถึงได้แม่นกว่าจุดเก่า โดยยังใช้ข้อมูลทั้งหน้าต่างช่วยลด noise เหมือนเดิม
// คืน true ถ้าผลสมเหตุสมผล (0.05 < k < 0.985 = เข้าหาสมดุลแบบเอ็กซ์โพเนนเชียลที่ไม่ช้าจนเดาไม่ได้)
bool fitAR1(const float* y, int n, float& vEq, float& tau) {
  if (n < 4) return false;
  // v15-fix: fitAR1() ถูกเรียกจากทั้งโหมด Predict (n สูงสุด PRED_MAX_SAMPLES = 20) และตัวตรวจแนวโน้มไหลช้า
  // (n สูงสุด TREND_MAX_SAMPLES = 40) — จำกัด n ที่ใช้จริงไม่ให้เกินขนาดอาเรย์ w[] ด้านล่าง กันเขียนล้นบัฟเฟอร์
  // (ผลคือถ้า n เกิน จะใช้แค่ FIT_AR1_MAX_N ตัวอย่างล่าสุด ซึ่งเพียงพออยู่แล้วสำหรับการฟิตทั้งสองกรณี)
  const int FIT_AR1_MAX_N = 40;
  if (n > FIT_AR1_MAX_N) { y += (n - FIT_AR1_MAX_N); n = FIT_AR1_MAX_N; }
  int m = n - 1;
  double sw = 0, sx = 0, sz = 0;
  double w[FIT_AR1_MAX_N];
  for (int i = 0; i < m; i++) {
    w[i] = pow((double)PRED_RECENCY_DECAY, (double)(m - 1 - i));  // i=m-1 (คู่ล่าสุด) -> น้ำหนัก 1.0
    sw += w[i]; sx += w[i] * y[i]; sz += w[i] * y[i + 1];
  }
  if (sw < 1e-10) return false;
  double mx = sx / sw, mz = sz / sw, sxx = 0, sxz = 0;
  for (int i = 0; i < m; i++) {
    double dx = y[i] - mx, dz = y[i + 1] - mz;
    sxx += w[i] * dx * dx;
    sxz += w[i] * dx * dz;
  }
  if (sxx < 1e-10) return false;
  double k = sxz / sxx;
  if (!(k > 0.05 && k < 0.985)) return false;
  double a = mz - k * mx;
  vEq = (float)(a / (1.0 - k));
  tau = (float)(-(PRED_SAMPLE_INTERVAL_MS / 1000.0) / log(k));
  return true;
}

// ---------- AR(2): y[n+2] = a + b1*y[n+1] + b2*y[n] (เอ็กซ์โพเนนเชียลสองตัว: ช่วงเร็ว + ช่วงช้า) ----------
// แก้สมการปกติ 3x3 ด้วย Gaussian elimination (ข้อมูลถูกเลื่อนศูนย์ด้วยค่าเฉลี่ยก่อนเพื่อให้เสถียรเชิงตัวเลข)
// ยอมรับเฉพาะกรณีรากทั้งสองเป็นจำนวนจริงบวก < 1 (เข้าหาสมดุลแบบโมโนโทนิก) — ไม่งั้นถือว่าใช้ไม่ได้
bool fitAR2(const float* y, int n, float& vEq, float& tau) {
  if (n < 8) return false;
  double mean = 0;
  for (int i = 0; i < n; i++) mean += y[i];
  mean /= n;
  int m = n - 2;
  double S[3][4];
  for (int p = 0; p < 3; p++) for (int q = 0; q < 4; q++) S[p][q] = 0;
  for (int i = 0; i < m; i++) {
    double u = y[i + 1] - mean, w = y[i] - mean, z = y[i + 2] - mean;
    double r[3] = { 1.0, u, w };
    for (int p = 0; p < 3; p++) {
      for (int q = 0; q < 3; q++) S[p][q] += r[p] * r[q];
      S[p][3] += r[p] * z;
    }
  }
  for (int c = 0; c < 3; c++) {
    int piv = c;
    for (int r = c + 1; r < 3; r++) if (fabs(S[r][c]) > fabs(S[piv][c])) piv = r;
    if (fabs(S[piv][c]) < 1e-12) return false;
    if (piv != c) for (int k = 0; k < 4; k++) { double t = S[c][k]; S[c][k] = S[piv][k]; S[piv][k] = t; }
    for (int r = c + 1; r < 3; r++) {
      double f = S[r][c] / S[c][c];
      for (int k = c; k < 4; k++) S[r][k] -= f * S[c][k];
    }
  }
  double x[3];
  for (int r = 2; r >= 0; r--) {
    double v = S[r][3];
    for (int k = r + 1; k < 3; k++) v -= S[r][k] * x[k];
    x[r] = v / S[r][r];
  }
  double a = x[0], b1 = x[1], b2 = x[2];
  double disc = b1 * b1 + 4.0 * b2;
  if (disc < 0) return false;
  double sq = sqrt(disc);
  double r1 = (b1 + sq) / 2.0, r2 = (b1 - sq) / 2.0;
  if (!(r1 > 0.05 && r1 < 0.985 && r2 > -0.05 && r2 < 0.985)) return false;
  double den = 1.0 - b1 - b2;
  if (fabs(den) < 0.005) return false;
  vEq = (float)(mean + a / den);
  double rmax = (r1 > r2) ? r1 : r2;
  tau = (float)(-(PRED_SAMPLE_INTERVAL_MS / 1000.0) / log(rmax));
  return true;
}

void predClear() {
  advMlClear();
  predictedEqRaw = NAN;
  predictedEqAw = NAN;
  predictedTauSec = NAN;
  predFlat = false;
  predConfident = false;
}

// คำนวณค่าสมดุลใหม่ทุกครั้งที่ได้ตัวอย่างบล็อกใหม่ (ดูคำอธิบายที่จุดประกาศตัวแปรด้านบน)
void computeEquilibriumPrediction() {
  int n = predN;
  if (n < PRED_MIN_SAMPLES) { predClear(); return; }
  float mn = predBuf[0], mx = predBuf[0];
  for (int i = 1; i < n; i++) { if (predBuf[i] < mn) mn = predBuf[i]; if (predBuf[i] > mx) mx = predBuf[i]; }
  float last = predBuf[n - 1];

  // กราฟแบนแล้ว: ไม่มีแนวโน้มให้ฟิต -> จุดสมดุล = ค่าเฉลี่ย 3 ตัวอย่างล่าสุด (เขียว)
  if ((mx - mn) < PRED_FLAT_RANGE) {
    int c3 = (n < 3) ? n : 3;
    float m3 = 0;
    for (int i = n - c3; i < n; i++) m3 += predBuf[i];
    m3 /= c3;
    predictedEqRaw = m3;
    predictedEqAw = awShownPeek(m3, currentTempC);   // v29: สเกลเดียวกับเส้นสด
    advMlClear();
    predictedTauSec = NAN;
    predFlat = true;
    predConfident = true;
    predInvalidStreak = 0;
    return;
  }

  float cand[3];
  int nc = 0;
  float e1 = 0, t1 = 0, e2 = 0, t2 = 0, e3 = 0, t3 = 0;
  bool ok1 = fitAR1(predBuf, n, e1, t1);                                            // ทั้งหน้าต่าง
  bool ok2 = (n >= PRED_TAIL_WINDOW + 1) &&
             fitAR1(predBuf + n - PRED_TAIL_WINDOW, PRED_TAIL_WINDOW, e2, t2);      // v15: หาง 12 ตัวอย่างล่าสุด (เดิม 8) — ตามหางเส้นโค้งได้ดีกว่าเมื่อช่วงต้นยังไม่ใช่เอ็กซ์โพเนนเชียล
  bool ok3 = fitAR2(predBuf, n, e3, t3);                                            // เอ็กซ์โพเนนเชียลสองตัว
  if (ok1) cand[nc++] = e1;
  if (ok2) cand[nc++] = e2;
  if (ok3) cand[nc++] = e3;
  if (nc == 0) { if (++predInvalidStreak >= 3) predClear(); return; }   // ผิดปกติชั่วครั้งชั่วคราวคงค่าเดิมไว้ ติดกัน 3 ครั้งค่อยล้าง

  // v15: เดิมเลือกวิธีเดียวเด็ดขาด (AR2 มาก่อนเสมอถ้ามี) ทำให้ค่าทำนายกระโดดทันทีที่ AR2 เพิ่ง fit ผ่าน/ไม่ผ่าน
  // ระหว่างบล็อก เปลี่ยนเป็นค่าเฉลี่ยถ่วงน้ำหนัก: AR(2) น้ำหนักสูงสุด (จับช่วงเร็ว+ช้าได้ ข้อมูลมากสุด),
  // AR(1) หางน้ำหนักรองลงมา (ตามพลวัตใกล้สมดุลปัจจุบันได้ไว), AR(1) เต็มหน้าต่างน้ำหนักต่ำสุด (เสถียรแต่ตามหางช้ากว่า)
  // -> การเปลี่ยนวิธีที่ใช้ได้/ใช้ไม่ได้ระหว่างบล็อกทำให้ค่าทำนายเปลี่ยนแบบราบรื่น ไม่กระโดดเป็นขั้นบันได
  double wSum = 0, vSum = 0;
  if (ok3) { wSum += 2.0; vSum += 2.0 * e3; }
  if (ok2) { wSum += 1.5; vSum += 1.5 * e2; }
  if (ok1) { wSum += 1.0; vSum += 1.0 * e1; }
  float vEq = (float)(vSum / wSum);
  float tau = ok3 ? t3 : (ok2 ? t2 : t1);   // tau/ETA ใช้จากวิธีที่มีลำดับความสำคัญสูงสุดที่ใช้ได้ (เส้นโค้งประวาดจากตัวเดียว ผสมกันไม่ได้ตรงไปตรงมา)
  if (!(vEq >= 0.0f && vEq <= 1.0f) || fabs(vEq - last) > 0.30f) { if (++predInvalidStreak >= 3) predClear(); return; }
  predInvalidStreak = 0;
  if (!(tau >= 1.0f)) tau = 1.0f;
  if (tau > 20000.0f) tau = 20000.0f;

  float cmn = cand[0], cmx = cand[0];
  for (int i = 1; i < nc; i++) { if (cand[i] < cmn) cmn = cand[i]; if (cand[i] > cmx) cmx = cand[i]; }
  predConfident = (nc >= 2) && ((cmx - cmn) <= PRED_CONFIRM_TOL);

  bool first = isnan(predictedEqRaw) || predFlat;   // ผสมกับค่าเดิมครึ่งต่อครึ่ง กันตัวเลขกระโดดทีละบล็อก
  predFlat = false;
  predictedEqRaw = first ? vEq : 0.5f * predictedEqRaw + 0.5f * vEq;
  predictedTauSec = (first || isnan(predictedTauSec)) ? tau : 0.5f * predictedTauSec + 0.5f * tau;
  predictedEqAw = awShownPeek(predictedEqRaw, currentTempC);   // v29: สเกลเดียวกับเส้นสด
  advMlUpdate(predBuf, n, vEq);   // v29: TinyML ค่าสมดุลรอง (ผู้สังเกตการณ์)
}

// เวลาที่คาดว่าจะเหลือจนเข้าใกล้จุดสมดุลในระยะ PRED_ETA_BAND (วินาที) — คืน -1 = ยังไม่ทราบ, 0 = ถึงแล้ว
float predEtaSec() {
  if (isnan(predictedEqRaw) || predN < 1) return -1.0f;
  float dist = fabs(predictedEqRaw - predBuf[predN - 1]);
  if (dist <= PRED_ETA_BAND) return 0.0f;
  if (isnan(predictedTauSec)) return -1.0f;
  float sec = predictedTauSec * log(dist / PRED_ETA_BAND);
  if (sec < 0) sec = 0;
  if (sec > 5999.0f) sec = 5999.0f;
  return sec;
}

// อัปเดตจอ LCD 16x2 ระหว่างโหมดทำนาย: บรรทัดแรก ค่าดิบปัจจุบัน -> aw ที่ทำนาย ("~" = ยังไม่มั่นใจ) / บรรทัดสอง ETA
void updatePredictLCD(float rawNow) {
  char l1[17], l2[17];
  if (isnan(predictedEqAw)) snprintf(l1, sizeof(l1), "raw:%.3f ?????", rawNow);
  else snprintf(l1, sizeof(l1), "raw:%.3f>%s%.3f", rawNow, predConfident ? "" : "~", predictedEqAw);
  float eta = predEtaSec();
  if (isnan(predictedEqAw)) {
    snprintf(l2, sizeof(l2), "Collect %d/%d", predN, PRED_MIN_SAMPLES);
  } else if (predFlat || eta == 0.0f) {
    snprintf(l2, sizeof(l2), "Reached EQ");
  } else if (eta < 0) {
    char etime[17];
    formatElapsedTimeLCD(predStartMs, etime, sizeof(etime));
    snprintf(l2, sizeof(l2), "%s", etime);
  } else {
    int mm = (int)eta / 60, ss = (int)eta % 60;
    snprintf(l2, sizeof(l2), "ETA %s%dm%02ds", predConfident ? "" : "~", mm, ss);
  }
  lcd.setCursor(0, 0);
  lcd.print(String(l1) + "                ");
  lcd.setCursor(0, 1);
  lcd.print(String(l2) + "                ");
}

// ---------- กราฟโหมด Predict ----------
void predGraphReset() {
  pgCap = (graphW * 68) / 100;              // ประวัติใช้พื้นที่ซ้าย ~68% ที่เหลือขวาไว้วาดเส้นโค้งที่ทำนาย
  if (pgCap < 20) pgCap = 20;
  if (pgCap > MAX_POINTS) pgCap = MAX_POINTS;
  pgCount = 0;
  pgStepMs = 500;
  pgSlotStartMs = millis();
  pgSlotSum = 0;
  pgSlotN = 0;
  pgScaleInit = false;
  pgLabelTopI = pgLabelBotI = -1;
}

// เก็บค่าเฉลี่ยทุก pgStepMs เป็น 1 จุดประวัติ; ครบความจุแล้วรวมทีละ 2 จุด (ความละเอียดครึ่งหนึ่ง เวลาครอบคลุมเป็น 2 เท่า)
void predGraphAccumulate(float v) {
  pgSlotSum += v;
  pgSlotN++;
  if (millis() - pgSlotStartMs < pgStepMs) return;
  float avg = pgSlotSum / pgSlotN;
  pgSlotSum = 0;
  pgSlotN = 0;
  pgSlotStartMs = millis();
  if (pgCount >= pgCap) {
    for (int i = 0; i < pgCap / 2; i++) pgHist[i] = 0.5f * (pgHist[2 * i] + pgHist[2 * i + 1]);
    pgCount = pgCap / 2;
    pgStepMs *= 2;
  }
  pgHist[pgCount++] = avg;
}

int pgY(float v) {
  float span = pgYmax - pgYmin;
  if (span < 1e-6f) span = 1e-6f;
  int y = graphY + (int)((pgYmax - v) / span * (graphH - 1));
  if (y < graphY) y = graphY;
  if (y > graphY + graphH - 1) y = graphY + graphH - 1;
  return y;
}

// คำอธิบายเส้นของโหมด Predict (แทน "Aw / Temp" ของกราฟวัดปกติ): เส้นทึบ = ค่าที่วัด, เส้นประม่วง = ค่าที่ทำนาย
void drawPredictLegend() {
  int ly = graphY - 12;
  tft.fillRect(0, ly - 6, 120, 14, COL_BG);
  tft.drawFastHLine(4, ly, 10, COL_LINE);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_LINE, COL_BG);
  tft.drawString("Aw", 16, ly - 4, 1);
  for (int x = 40; x < 52; x += 4) tft.drawFastHLine(x, ly, 2, TFT_MAGENTA);
  tft.setTextColor(TFT_MAGENTA, COL_BG);
  tft.drawString("Predict", 56, ly - 4, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
}

// ตัวเลขแกน Y ซ้ายมือ (สเกลซูมอัตโนมัติ) แสดงแบบไม่มีเลข 0 นำหน้า เช่น ".753" ให้พอดีขอบซ้าย — วาดใหม่เฉพาะตอนสเกลเปลี่ยน
void drawPredictAxisLabels() {
  int topI = (int)(pgYmax * 1000.0f + 0.5f), botI = (int)(pgYmin * 1000.0f + 0.5f);
  if (topI == pgLabelTopI && botI == pgLabelBotI) return;
  pgLabelTopI = topI;
  pgLabelBotI = botI;
  tft.fillRect(0, graphY - 5, graphX - 2, graphH + 10, COL_BG);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(MR_DATUM);
  float vals[3] = { pgYmax, 0.5f * (pgYmax + pgYmin), pgYmin };
  int ys[3] = { graphY, graphY + graphH / 2, graphY + graphH };
  for (int i = 0; i < 3; i++) {
    char b[10];
    snprintf(b, sizeof(b), "%.3f", vals[i]);
    tft.drawString((b[0] == '0') ? b + 1 : b, graphX - 3, ys[i], 1);
  }
  tft.setTextDatum(TL_DATUM);
}

// วาดกราฟโหมด Predict: เส้นทึบ = ประวัติ, เส้นประนอนม่วง = จุดสมดุลที่ทำนาย, เส้นโค้งประม่วงต่อจากปัจจุบัน = คาดว่าจะไปทางไหน
// วงกลมเล็กบนเส้นโค้ง = จุดที่คาดว่าจะเข้าใกล้สมดุล (ETA) — เขียว = กราฟแบนถึงสมดุลแล้ว
void drawPredictGraph(int holdPhase) {
  float live = isnan(graphEmaAw) ? currentAw : graphEmaAw;
  bool haveEq = !isnan(predictedEqAw);

  // สเกล Y ซูมอัตโนมัติ: ครอบข้อมูลทั้งหมด + จุดสมดุลที่ทำนาย + ขอบเผื่อ (ขยายทันทีถ้าข้อมูลหลุดกรอบ หดช้าๆ กันสเกลสั่น)
  float lo = live, hi = live;
  for (int i = 0; i < pgCount; i++) { if (pgHist[i] < lo) lo = pgHist[i]; if (pgHist[i] > hi) hi = pgHist[i]; }
  if (haveEq) { if (predictedEqAw < lo) lo = predictedEqAw; if (predictedEqAw > hi) hi = predictedEqAw; }
  float margin = (hi - lo) * 0.25f;
  if (margin < 0.004f) margin = 0.004f;
  float tmin = lo - margin, tmax = hi + margin;
  if (tmax - tmin < 0.02f) { float mid = 0.5f * (tmax + tmin); tmin = mid - 0.01f; tmax = mid + 0.01f; }
  if (tmin < 0.0f) { tmax -= tmin; tmin = 0.0f; }
  if (tmax > 1.0f) { tmin -= (tmax - 1.0f); tmax = 1.0f; if (tmin < 0.0f) tmin = 0.0f; }
  if (!pgScaleInit) { pgYmin = tmin; pgYmax = tmax; pgScaleInit = true; }
  else {
    if (tmin < pgYmin) pgYmin = tmin; else pgYmin += 0.05f * (tmin - pgYmin);
    if (tmax > pgYmax) pgYmax = tmax; else pgYmax += 0.05f * (tmax - pgYmax);
  }

  tft.fillRect(graphX, graphY, graphW, graphH, COL_BG);
  tft.drawFastHLine(graphX, graphY, graphW, COL_GRID);
  tft.drawFastHLine(graphX, graphY + graphH / 2, graphW, COL_GRID);
  tft.drawFastHLine(graphX, graphY + graphH - 1, graphW, COL_GRID);

  int xEnd = graphX + graphW - 1;
  int xNow = graphX + pgCount;
  if (xNow > graphX + pgCap) xNow = graphX + pgCap;
  for (int y = graphY; y < graphY + graphH; y += 4) tft.drawPixel(xNow, y, COL_GRID);   // เส้นแบ่ง "ตอนนี้"

  // ประวัติ (เส้นทึบ) + ต่อไปยังค่าปัจจุบัน
  uint16_t lineColor = (holdPhase == 2) ? COL_OK : COL_LINE;
  for (int i = 0; i < pgCount - 1; i++) tft.drawLine(graphX + i, pgY(pgHist[i]), graphX + i + 1, pgY(pgHist[i + 1]), lineColor);
  if (pgCount > 0) tft.drawLine(graphX + pgCount - 1, pgY(pgHist[pgCount - 1]), xNow, pgY(live), lineColor);

  if (haveEq) {
    uint16_t pc = predFlat ? COL_OK : TFT_MAGENTA;
    int yEq = pgY(predictedEqAw);
    for (int x = graphX; x <= xEnd; x += 6) tft.drawFastHLine(x, yEq, 3, pc);   // เส้นประนอน = จุดสมดุลที่ทำนาย

    if (!isnan(predictedTauSec) && !predFlat) {
      int xPrev = xNow, yPrev = pgY(live);
      for (int x = xNow + 1; x <= xEnd; x++) {
        float tSec = (float)(x - xNow) * (float)pgStepMs / 1000.0f;   // แกนเวลาเดียวกับประวัติ (px ละ pgStepMs)
        float v = predictedEqAw + (live - predictedEqAw) * expf(-tSec / predictedTauSec);
        int y = pgY(v);
        if (((x - xNow) / 3) % 2 == 0) tft.drawLine(xPrev, yPrev, x, y, pc);   // เส้นโค้งประ
        xPrev = x;
        yPrev = y;
      }
      float eta = predEtaSec();
      if (eta > 0.0f) {
        int xe = xNow + (int)(eta * 1000.0f / (float)pgStepMs);
        if (xe <= xEnd) tft.drawCircle(xe, pgY(predictedEqAw + (live - predictedEqAw) * expf(-eta / predictedTauSec)), 3, pc);
      }
    }

    char lb[16];
    snprintf(lb, sizeof(lb), "EQ%s%.3f", predConfident ? " " : "~", predictedEqAw);
    tft.setTextDatum(TR_DATUM);
    tft.setTextColor(pc, COL_BG);
    int ly = (yEq - 11 >= graphY) ? (yEq - 11) : (yEq + 3);
    tft.drawString(lb, xEnd - 2, ly, 1);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(COL_TEXT, COL_BG);
  }
  drawPredictAxisLabels();
}

// บรรทัดล่างสุดของจอ TFT ในโหมด Predict (แทนเวลาอย่างเดียวของโหมดวัดปกติ): เวลา / จุดสมดุลที่ทำนาย / ETA
void drawPredictInfoTFT() {
  int labelY = graphY + graphH + 2;
  tft.fillRect(0, labelY, screenW, screenH - labelY, COL_BG);
  unsigned long el = (millis() - predStartMs) / 1000UL;
  unsigned mm = (unsigned)(el / 60UL), ss = (unsigned)(el % 60UL);
  if (mm > 99) mm = 99;
  char buf[64];
  if (isnan(predictedEqAw)) {
    snprintf(buf, sizeof(buf), "Time %02u:%02u  Collecting %d/%d", mm, ss, predN, PRED_MIN_SAMPLES);
  } else {
    char etaS[16];
    float eta = predEtaSec();
    if (predFlat || eta == 0.0f) snprintf(etaS, sizeof(etaS), "REACHED");
    else if (eta < 0) snprintf(etaS, sizeof(etaS), "ETA --");
    else snprintf(etaS, sizeof(etaS), "ETA %s%dm%02ds", predConfident ? "" : "~", (int)eta / 60, (int)eta % 60);
    if (screenW >= 240) snprintf(buf, sizeof(buf), "Time %02u:%02u  EQ%s%.3f  %s", mm, ss, predConfident ? " " : "~", predictedEqAw, etaS);
    else snprintf(buf, sizeof(buf), "EQ%s%.3f %s", predConfident ? " " : "~", predictedEqAw, etaS);
  }
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(buf, screenW / 2, labelY, 1);
  tft.setTextDatum(TL_DATUM);
}

void runPredictAWTick() {
  if (millis() - predLastUpdate < updateInterval) return;
  predLastUpdate = millis();

  float raw;
  currentAw = readAwAndRaw(raw);
  pushValue(currentAw, currentTempC);   // ใช้ตัวกรอง EMA เดียวกับกราฟวัดปกติ (graphEmaAw) เป็นค่าที่วาด

  // สะสมค่าดิบเป็นบล็อก แล้วปิดบล็อกทุก PRED_SAMPLE_INTERVAL_MS -> ตัวอย่าง (ค่าเฉลี่ย) 1 ตัวสำหรับสมการทำนาย
  predBlockSum += raw;
  predBlockCnt++;
  if (millis() - predBlockStartMs >= PRED_SAMPLE_INTERVAL_MS && predBlockCnt > 0) {
    float avg = (float)(predBlockSum / predBlockCnt);
    if (predN >= PRED_MAX_SAMPLES) {
      for (int i = 0; i < PRED_MAX_SAMPLES - 1; i++) predBuf[i] = predBuf[i + 1];
      predN = PRED_MAX_SAMPLES - 1;
    }
    predBuf[predN++] = avg;
    predBlockSum = 0;
    predBlockCnt = 0;
    predBlockStartMs = millis();
    computeEquilibriumPrediction();
  }

  predGraphAccumulate(isnan(graphEmaAw) ? currentAw : graphEmaAw);
  int holdPhase = isnan(predictedEqAw) ? 0 : (predFlat ? 2 : 1);   // แค่สีไฮไลต์ ไม่ได้ล็อกค่าใด ๆ
  updateMeasureLEDs(holdPhase);   // v16: โหมด Predict มีไฟสถานะด้วย: แดง=ยังทำนายไม่ได้ / เหลือง=มีค่าทำนาย (ยังไหลอยู่) / เขียว=ถึงสมดุลแล้ว
  drawPredictGraph(holdPhase);
  drawCurrentValue(currentAw, holdPhase);
  drawCurrentTemp(currentTempC);
  drawPredictInfoTFT();
  updatePredictLCD(raw);
}

// เข้าสู่โหมดเปรียบเทียบ 2 ตัวอย่าง (เมนู "1.3 Compare") เริ่มจากตัวอย่าง A ก่อนเสมอ
void enterCompareAW() {
  compareStage = 0;
  compareAwA = NAN;
  compareAwB = NAN;
  compareSaved = false;
  startSensorConditioning(true, COND_NEXT_COMPARE_A, 2);  // v12: ฮีต/รอเย็นก่อนวัดตัวอย่าง A
}

void beginCompareA() {
  compareStage = 0;
  state = ST_COMPARE_MEASURE;
  numPoints = 0;
  resetGraphFilter();
  iconVisible = false;
  savePromptActive = false;
  resetStabilityWindow();
  tft.fillScreen(COL_BG);
  drawStaticUI();
  lastUpdate = 0;
  lcd.clear();
}

// เริ่มวัดตัวอย่าง B (เรียกหลังผู้ใช้ตอบ Yes ตอน A นิ่ง และฮีต/รอเย็นเซนเซอร์เสร็จแล้ว)
void beginCompareB() {
  compareStage = 1;
  state = ST_COMPARE_MEASURE;
  numPoints = 0;
  resetGraphFilter();
  iconVisible = false;
  savePromptActive = false;
  resetStabilityWindow();
  tft.fillScreen(COL_BG);
  drawStaticUI();
  lastUpdate = 0;
  lcd.clear();
}

// บรรทัด LCD ระหว่างวัดในโหมดเปรียบเทียบ: ติดป้าย [A]/[B] ให้รู้ว่ากำลังวัดตัวอย่างไหนอยู่
void updateCompareLCD(float v, int holdPhase, int idx) {
  lcd.setCursor(0, 0);
  char tag = (compareStage == 0) ? 'A' : 'B';
  char statCh = (holdPhase == 2) ? 'H' : (holdPhase == 1) ? 'Y' : '-';
  char l1[17];
  snprintf(l1, sizeof(l1), "[%c]aw:%.3f %c", tag, v, statCh);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  // v11: ตัดบรรทัดหมวดอาหารออกจากจอ LCD -> โชว์ระยะเวลาที่กำลังวัดตัวอย่างนี้อยู่แทน
  // (measureStartMs ถูกรีเซ็ตทุกครั้งที่เริ่มวัดตัวอย่างใหม่ผ่าน resetStabilityWindow() อยู่แล้ว
  // ทั้งตอนเริ่มตัวอย่าง A และตอนสลับไปตัวอย่าง B จึงนับเวลาของตัวอย่างที่กำลังวัดอยู่ ณ ขณะนั้นได้ถูกต้อง)
  (void)idx; // ไม่ใช้หมวดอาหารในบรรทัดนี้อีกต่อไป แต่ยังรับพารามิเตอร์ไว้เผื่อใช้ที่อื่นในอนาคต
  char l2[17];
  formatElapsedTimeLCD(measureStartMs, l2, sizeof(l2));
  String lbl = String(l2);
  while (lbl.length() < 16) lbl += ' ';
  lcd.print(lbl.substring(0, 16));
}

// ทำงานทุก tick ระหว่างโหมดเปรียบเทียบ — ใช้ตรรกะ "นิ่ง" ชุดเดียวกับ runMeasureAWTick ทุกประการ
// v12: เมื่อกราฟนิ่ง จะเด้งป็อปอัปทับกราฟโดย "ยังวัดต่อ" จนกว่าผู้ใช้จะเลือก (ดู handleComparePromptChoice)
//   ตัวอย่าง A: "Measure B next?"  Yes = ไปวัด B ต่อ / No = ยกเลิกโหมดเปรียบเทียบ
//   ตัวอย่าง B: "Save A and B?"    แล้วไปหน้าสรุปผลเทียบ A vs B
// (แทนหน้าเต็มจอ "SAMPLE A STABLE / Continue to sample B?" เดิมที่หยุดวัดไปเลย)
void runCompareTick() {
  if (millis() - lastUpdate < updateInterval) return;
  lastUpdate = millis();

  float raw;
  currentAw = readAwAndRaw(raw);
  pushValue(currentAw, currentTempC);
  updateStabilityWindow(raw, currentTempC);

  if (measureStable && !savePromptActive) {
    savePromptActive = true;
    promptSel = 0;
    refreshPromptValues();
  }
  if (savePromptActive) refreshPromptValues();

  // v16: สีเดียวกันทุกโหมด อ่านจากตัวตรวจนิ่งกลาง (stabPhase: 0 แดง=ยังเคลื่อนที่ / 1 เหลือง=คงที่ 20 วิ / 2 เขียว=ล็อกได้)
  int holdPhase = stabPhase;
  // v22: เหลืองอยู่แต่ช่วงกว้างหน้าต่าง 60 วิแคบจนเกือบผ่านเกณฑ์ล็อกแล้ว (แค่รอเวลา/ความชัน) = ใกล้นิ่งมาก -> เขียวกระพริบ
  bool nearStable = (holdPhase == 1) && !isnan(stabRangeAw) && (stabRangeAw <= STAB_TOL * 1.3f);

  updateMeasureLEDs(holdPhase, nearStable);
  drawGraph(holdPhase);
  drawCurrentValue(currentAw, holdPhase);
  drawCurrentTemp(currentTempC);
  int idx = getFoodCategoryIndex(currentAw);
  drawElapsedTimeTFT(measureStartMs);
  if (savePromptActive) {
    drawPromptPopupTFT();
    drawPromptLCD();
  } else {
    updateCompareLCD(currentAw, holdPhase, idx);
  }
}

void drawCompareResultScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  char l1[17];
  snprintf(l1, sizeof(l1), "A:%.3f B:%.3f", compareAwA, compareAwB);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  char l2[17];
  snprintf(l2, sizeof(l2), compareSaved ? "D:%+.3f Saved" : "D:%+.3f NoSave", compareAwB - compareAwA);  // v12: บันทึก/ไม่บันทึกไปแล้วตอนตอบป็อปอัปของ B
  lcd.print(l2);

  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("COMPARE RESULT", screenW / 2, 4, 2);

  char diffBuf[28];
  snprintf(diffBuf, sizeof(diffBuf), "Diff (B-A): %+.3f", compareAwB - compareAwA);
  tft.drawString(diffBuf, screenW / 2, 26, 1);

  int midX = screenW / 2;
  int barBaseY = screenH - 40;
  int barMaxH = barBaseY - 46;
  if (barMaxH < 10) barMaxH = 10;
  int barW = screenW / 8;
  if (barW < 16) barW = 16;

  int hA = constrain((int)(compareAwA * barMaxH), 2, barMaxH);
  int hB = constrain((int)(compareAwB * barMaxH), 2, barMaxH);

  int xA = midX - (screenW / 5) - barW / 2;
  int xB = midX + (screenW / 5) - barW / 2;

  tft.fillRect(xA, barBaseY - hA, barW, hA, foodCategories[compareCatA].color);
  tft.drawRect(xA, barBaseY - hA, barW, hA, COL_TEXT);
  tft.fillRect(xB, barBaseY - hB, barW, hB, foodCategories[compareCatB].color);
  tft.drawRect(xB, barBaseY - hB, barW, hB, COL_TEXT);
  tft.drawFastHLine(0, barBaseY, screenW, COL_AXIS);

  tft.setTextColor(COL_TEXT, COL_BG);
  char bufA[14], bufB[14];
  snprintf(bufA, sizeof(bufA), "A:%.3f", compareAwA);
  snprintf(bufB, sizeof(bufB), "B:%.3f", compareAwB);
  tft.drawString(bufA, xA + barW / 2, barBaseY + 4, 1);
  tft.drawString(bufB, xB + barW / 2, barBaseY + 4, 1);

  tft.setTextColor(foodCategories[compareCatA].color, COL_BG);
  tft.drawString(foodCategories[compareCatA].label, xA + barW / 2, barBaseY - hA - 12, 1);
  tft.setTextColor(foodCategories[compareCatB].color, COL_BG);
  tft.drawString(foodCategories[compareCatB].label, xB + barW / 2, barBaseY - hB - 12, 1);

  tft.setTextColor(compareSaved ? COL_OK : COL_WARN, COL_BG);
  tft.drawString(compareSaved ? "SAVED (A+B)  -  press any to exit" : "NOT SAVED  -  press any to exit", midX, screenH - 12, 1);
  tft.setTextDatum(TL_DATUM);
}

// จุดไข่ปลาแสดงตำแหน่งเมนูปัจจุบัน (เหมือน page indicator) พร้อมเด้งเข้าตำแหน่งใหม่
void drawMenuDots(int count, int sel) {
  if (count <= 1) return;
  const int spacing = 12;
  int totalW = (count - 1) * spacing;
  int startX = screenW / 2 - totalW / 2;
  int y = 18;
  for (int i = 0; i < count; i++) {
    int rr = (i == sel) ? 3 : 2;
    uint16_t col = (i == sel) ? COL_TOUCH : COL_AXIS;
    tft.fillCircle(startX + i * spacing, y, rr, col);
  }
}

// แถบคำแนะนำปุ่มด้านล่างจอ ช่วยให้ผู้ใช้ใหม่รู้วิธีกดโดยไม่ต้องเดา
void drawButtonHint(const char* text) {
  tft.fillRect(0, screenH - 11, screenW, 11, COL_BG);
  tft.setTextDatum(BC_DATUM);
  tft.setTextColor(TFT_DARKGREY, COL_BG);
  tft.drawString(text, screenW / 2, screenH - 1, 1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
}

void drawMenuScreen(const char* title, const char* const* items, const IconType* icons, int count, int sel) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(String(title).substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print((">" + String(items[sel])).substring(0, 16));

  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(title, screenW / 2, 4, 2);
  drawMenuDots(count, sel);
  // หน้าเมนูหลักใช้มาสคอต RuiBo แทนไอคอนทั่วไป ส่วนเมนูย่อยอื่น ๆ ยังใช้ไอคอนเดิมเพื่อบอกฟังก์ชันชัดเจน
  if (String(title) == "MAIN MENU") drawMascotCenter(items[sel]);
  else drawCenterIcon(icons[sel], items[sel]);
  drawButtonHint("UP/DOWN Move   HOLD Select   BOTH Back");
  tft.setTextDatum(TL_DATUM);
}

// วาด QR code ที่ตำแหน่ง (x0,y0) ให้พอดีกรอบสี่เหลี่ยมขนาดไม่เกิน maxSize พิกเซล
// เข้ารหัสเป็นลิงก์ "http://<IP>/" ของหน้าเว็บเครื่อง มือถือสแกนแล้วเปิดเบราว์เซอร์ไปที่แดชบอร์ดได้เลย
// เวอร์ชัน QR สูงสุดที่อนุญาต และขนาด buffer ที่ต้องใช้ให้ตรงกัน (ต้อง <= qrcodegen_VERSION_MAX)
#define QR_MAX_VERSION 10
#define QR_BUF_LEN qrcodegen_BUFFER_LEN_FOR_VERSION(QR_MAX_VERSION)
int drawWifiQRCode(int x0, int y0, int maxSize) {
  // QR ตอนนี้เข้ารหัสเป็น "WIFI:T:WPA;S:<ssid>;P:<password>;;" ตามมาตรฐาน Wi-Fi QR ทั่วไป
  // มือถือ (กล้อง/สแกนเนอร์ปกติ) สแกนแล้วจะเสนอ "เชื่อมต่อ Wi-Fi" ให้ทันที ไม่ต้องพิมพ์รหัสผ่านเอง
  // หลังต่อ Wi-Fi ติดแล้ว ผู้ใช้ค่อยเปิดเบราว์เซอร์ไปที่ IP ที่แสดงบนจอ (หรือสแกนซ้ำ/พิมพ์เอง) เพื่อเข้าแดชบอร์ด
  // หมายเหตุ: ตัวอักษรพิเศษ ; , : \ ในชื่อ SSID/รหัสผ่านต้องใส่ \ นำหน้า (escape) ตามสเปค — ที่นี่ค่าเป็นตัวอักษร/ตัวเลขล้วนจึงไม่ต้อง escape
  char payload[96];
  snprintf(payload, sizeof(payload), "WIFI:T:WPA;S:%s;P:%s;;", ssid, wifiPassword);

  // qrcodegen: ให้ไลบรารีเลือกเวอร์ชัน (ขนาด) เล็กที่สุดที่ใส่ข้อมูลพอดีเองในช่วง VERSION_MIN..VERSION_MAX
  // ที่ ECC_LOW เป็นจุดเริ่ม (boostEcl=true ให้ไลบรารียกระดับ ECC ขึ้นเองถ้าเวอร์ชันเดิมยังใส่ได้ ทำให้สแกนทนขึ้นฟรี ๆ)
  // FIX (บอร์ดค้างแล้วรีบูตตอนเข้าหน้า WiFi Info): เดิมประกาศ buffer ขนาด qrcodegen_BUFFER_LEN_MAX (เวอร์ชัน 40 =
  // 3,918 ไบต์) จำนวน 2 ก้อนไว้ "บนสแต็ก" รวม ~7.8 KB ขณะที่สแต็กของ loop() บน ESP32 มีแค่ 8 KB เป็นค่าเริ่มต้น
  // -> สแต็กล้น (Stack canary watchpoint triggered) บอร์ดรีเซ็ตทันทีที่วาด QR ทั้งที่ QR จริงมีแค่ 29x29 โมดูล
  // วิธีแก้: จำกัดเวอร์ชันสูงสุดเหลือ QR_MAX_VERSION (10 = 57x57 โมดูล ใส่ข้อความได้ ~174 ไบต์ที่ ECC ต่ำ
  // เกินพอสำหรับลิงก์ http://...) ทำให้ buffer เหลือก้อนละ ~408 ไบต์ และย้ายไปเป็น static (อยู่ใน BSS ไม่กินสแต็กเลย)
  // ผลลัพธ์ QR เหมือนเดิมทุกประการ เพราะไลบรารีเลือกเวอร์ชัน "เล็กที่สุดที่พอดี" อยู่แล้ว (ลิงก์ http://192.168.4.1/ ได้เวอร์ชัน 2 = 25x25 โมดูล)
  static uint8_t qr0[QR_BUF_LEN];
  static uint8_t qrTempBuffer[QR_BUF_LEN];
  bool qrOk = qrcodegen_encodeText(payload, qrTempBuffer, qr0, qrcodegen_Ecc_LOW,
                                    qrcodegen_VERSION_MIN, QR_MAX_VERSION,
                                    qrcodegen_Mask_AUTO, true);
  if (!qrOk) return 0; // payload ยาวเกินกว่าจะเข้ารหัสเป็น QR ได้ (ไม่ควรเกิดกับลิงก์สั้น ๆ แบบนี้)

  // ขยาย QR ให้ใหญ่ที่สุดเท่าที่กล่อง maxSize x maxSize จะรับได้: 1 โมดูล = px พิกเซล (จำนวนเต็มเสมอ เพื่อให้ขอบคม
  // สแกนง่าย) และกันขอบขาว (quiet zone) ไว้ข้างละ 2 โมดูล รวมเป็น (modules + 4) โมดูลต่อด้าน
  // ตัวอย่าง: QR 25x25 ในกล่อง 180 px -> px = 180/29 = 6
  const int QUIET_MODULES = 2;
  int modules = qrcodegen_getSize(qr0);
  int px = maxSize / (modules + QUIET_MODULES * 2);
  if (px < 1) px = 1;
  int quiet = px * QUIET_MODULES;
  int totalSize = px * modules + quiet * 2;

  // จัดให้อยู่กึ่งกลางกล่องในแนวนอน (ชิดบน) แล้ววาดพื้นขาวรวมขอบก่อน จากนั้นวาดเฉพาะโมดูลสีดำทับ
  int left = x0 + (maxSize - totalSize) / 2;
  tft.fillRect(left, y0, totalSize, totalSize, TFT_WHITE);
  for (int y = 0; y < modules; y++) {
    for (int x = 0; x < modules; x++) {
      if (qrcodegen_getModule(qr0, x, y)) {
        tft.fillRect(left + quiet + x * px, y0 + quiet + y * px, px, px, TFT_BLACK);
      }
    }
  }
  return totalSize;
}

void drawWifiInfoScreen() {
  String ip = WiFi.softAPIP().toString();
  const char* calRowTxt = AW_RAW_MODE ? "Disabled (RAW)" : "Editable (web)";

  // ---- LCD 16x2 ----
  lcd.clear();
  lcd.setCursor(0, 0);
  String l1 = "S:" + String(ssid);
  while (l1.length() < 16) l1 += ' ';
  lcd.print(l1.substring(0, 16));
  lcd.setCursor(0, 1);
  String l2 = "IP:" + ip;
  while (l2.length() < 16) l2 += ' ';
  lcd.print(l2.substring(0, 16));

  // ---- TFT ----
  // เปลี่ยนผังหน้าใหม่: เดิมข้อความ (SSID/Password/IP/Calibration) วางเป็นคอลัมน์ซ้าย 4 บรรทัด
  // แล้วให้ QR หดตัวไปแบ่งพื้นที่ทางขวา ทำให้ QR เล็กลงมากตามความกว้างของข้อความที่เหลือ
  // ผังใหม่: ย่อข้อความทั้งหมดให้เป็นแถบสรุปแนวนอนบาง ๆ ไว้บนสุด (เต็มความกว้างจอ, ใช้ฟอนต์เล็กลง)
  // แล้วปล่อยพื้นที่ที่เหลือเกือบทั้งหมดของจอให้ QR — ทำให้ QR ใหญ่ขึ้นกว่าเดิมมาก โดยยังอ่านค่าครบทุกอย่างเหมือนเดิม
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("WIFI INFO", screenW / 2, 4, 2);

  tft.setTextDatum(TL_DATUM);
  int x = 6;
  int y = 22;
  const int lh = 12; // แถบสรุปใช้ฟอนต์เล็ก (font 1) บรรทัดถี่กว่าเดิมมาก เพื่อประหยัดพื้นที่แนวตั้งให้ QR

  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("SSID:", x, y, 1);
  tft.setTextColor(TFT_CYAN, COL_BG);
  tft.drawString(ssid, x + tft.textWidth("SSID: ", 1), y, 1);

  y += lh;
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Pass:", x, y, 1);
  tft.setTextColor(TFT_CYAN, COL_BG);
  tft.drawString(wifiPassword, x + tft.textWidth("Pass: ", 1), y, 1);

  y += lh;
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("IP:", x, y, 1);
  tft.setTextColor(COL_OK, COL_BG);
  tft.drawString(ip.c_str(), x + tft.textWidth("IP: ", 1), y, 1);

  y += lh;
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Cal:", x, y, 1);
  tft.setTextColor(COL_OK, COL_BG);
  tft.drawString(calRowTxt, x + tft.textWidth("Cal: ", 1), y, 1);

  // QR code เต็มความกว้างจอ อยู่กึ่งกลาง ใต้แถบสรุปข้อความด้านบน
  // สแกนแล้วเชื่อมต่อ Wi-Fi ให้อัตโนมัติ (ไม่ต้องพิมพ์รหัสผ่านเอง) — ดูรายละเอียดใน drawWifiQRCode()
  const int qrTop = y + lh + 6;    // ใต้บรรทัดสุดท้ายของแถบสรุป
  const int labelsH = 22;          // พื้นที่ข้อความ "Scan to / join WiFi" ใต้ QR
  const int exitH = 14;            // พื้นที่ข้อความ "Press to exit" ท้ายจอ
  const int sideMargin = 6;        // ขอบซ้าย-ขวาแคบ ๆ กันโมดูล QR ชิดขอบจอเกินไป
  int availH = screenH - qrTop - labelsH - exitH;
  int availW = screenW - sideMargin * 2;
  int qrBox = (availW < availH) ? availW : availH;
  if (qrBox < 48) qrBox = 48;  // กันจอเล็กมากจน QR เละจนสแกนไม่ได้
  int qrX = (screenW - qrBox) / 2;
  int qrY = qrTop;
  int qrDrawn = drawWifiQRCode(qrX, qrY, qrBox);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Scan to", screenW / 2, qrY + qrDrawn + 2, 1);
  tft.drawString("join WiFi", screenW / 2, qrY + qrDrawn + 12, 1);
  tft.setTextDatum(TL_DATUM);

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Press to exit", screenW / 2, screenH - 12, 1);
  tft.setTextDatum(TL_DATUM);
}

// v-pro: หน้า "System Health" รวมศูนย์ — โชว์สถานะเซนเซอร์/Wi-Fi/นาฬิกา/อายุคาลิเบรตทั้งหมดพร้อมกันในหน้าเดียว
// (แก้ช่องว่างที่ระบุไว้ในสเปคว่า "ไม่มีหน้าจอสรุป error/สถานะเซนเซอร์แบบรวมศูนย์")
void drawSysHealthScreen() {
  bool calDue = false;
  uint32_t calAgeDays = 0;
  if (calSavedAtEpoch > 0 && clockEverSynced) {
    time_t age = nowEpoch() - calSavedAtEpoch;
    if (age > 0) calAgeDays = (uint32_t)(age / 86400UL);
    calDue = calAgeDays >= CAL_REMINDER_DAYS;
  }
  bool anyFault = sensorFaultSHT || sensorFaultDS18B20 || wifiApFault || calDue || lastMeasureNoiseWarning || coldRoomWarn;

  // ---- LCD 16x2: สรุปแบบย่อสุด ----
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(anyFault ? "SYS HEALTH: WARN" : "SYS HEALTH: OK  ");
  lcd.setCursor(0, 1);
  if (sensorFaultSHT) lcd.print("RH sensor fault");
  else if (sensorFaultDS18B20) lcd.print("Temp sensor fault");
  else if (coldRoomWarn) lcd.print("Room colder <25C");
  else if (calDue) lcd.print("Calibration due!");
  else if (lastMeasureNoiseWarning) lcd.print("Last read noisy");
  else lcd.print("All systems OK ");

  // ---- TFT: รายการสถานะทีละแถว พร้อมสีเขียว/แดงชัดเจน ----
  tft.fillScreen(COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("SYSTEM HEALTH", screenW / 2, 4, 2);
  tft.setTextDatum(TL_DATUM);

  int x = 10, y = 30, lh = 15;
  auto row = [&](const char* label, bool ok, const char* okTxt, const char* badTxt) {
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString(label, x, y, 1);
    tft.setTextColor(ok ? COL_OK : COL_WARN, COL_BG);
    tft.drawString(ok ? okTxt : badTxt, x + 96, y, 1);
    y += lh;
  };
  { // v17: ป้ายชื่อแถวนี้บอกด้วยว่าตอนนี้บอร์ดตรวจพบ/กำลังใช้เซนเซอร์ตัวไหนอยู่ (SHT/DHT/NONE)
    char humLabel[34];
    snprintf(humLabel, sizeof(humLabel), "Humidity sensor (%s):", humSensorTypeName());
    row(humLabel, !sensorFaultSHT, "OK", "FAULT");
  }
  row("Temp sensor (DS18B20):", !sensorFaultDS18B20, "OK", "FAULT");
  row("Wi-Fi Access Point:", !wifiApFault, "OK", "FAULT");
  row("Room temp vs Peltier:", !peltierStuckHot, "OK", "TOO HOT");
  row("Chamber vs target:", !coldRoomWarn, "OK", "TOO COLD");   // v14: ห้องแอร์เย็นกว่าเป้าหมาย เทลเทียร์ทำความร้อนไม่ได้
  {
    // v14: ส่วนต่างอุณหภูมิ ตัวชิป SHT - ตัวอย่าง (DS18B20) — เกิน ~1 °C ค่า RH เริ่มคลาด (ชดเชยให้อัตโนมัติในโหมดคาลิเบรต)
    float dT = gradientDeltaC();
    char db[20];
    if (isnan(dT)) snprintf(db, sizeof(db), "n/a");
    else snprintf(db, sizeof(db), "%+.1f C%s", dT, (fabs(dT) > 1.0f) ? " CHECK" : "");
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString("SHT-sample dT:", x, y, 1);
    tft.setTextColor((isnan(dT) || fabs(dT) <= 1.0f) ? COL_OK : COL_WARN, COL_BG);
    tft.drawString(db, x + 96, y, 1);
    y += lh;
  }
  row("Last reading noise:", !lastMeasureNoiseWarning, "OK", "CHECK SENSOR");

  char calLabel[24];
  if (calSavedAtEpoch == 0) {
    snprintf(calLabel, sizeof(calLabel), "never set");
  } else if (!clockEverSynced) {
    snprintf(calLabel, sizeof(calLabel), "set (age unknown)");
  } else {
    snprintf(calLabel, sizeof(calLabel), "%lu days ago", (unsigned long)calAgeDays);
  }
#if AW_RAW_MODE
  snprintf(calLabel, sizeof(calLabel), "RAW (not used)");
#endif
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Calibration:", x, y, 1);
  tft.setTextColor(calDue ? COL_WARN : COL_OK, COL_BG);
  tft.drawString(calLabel, x + 96, y, 1);
  y += lh;

  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Clock (audit trail):", x, y, 1);
  tft.setTextColor(clockEverSynced ? (clockVerifiedThisBoot ? COL_OK : COL_TEMP) : COL_WARN, COL_BG);
  tft.drawString(clockEverSynced ? (clockVerifiedThisBoot ? "SYNCED" : "STALE") : "UNSET", x + 96, y, 1);
  y += lh;

  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Operator:", x, y, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(strlen(operatorTag) > 0 ? operatorTag : "(not set)", x + 96, y, 1);
  y += lh;

  {
    // v13: สาเหตุรีเซ็ตล่าสุด — BROWNOUT/WDT/PANIC = ไฟตก/โค้ดค้าง (สีแดง) ต่อ ให้เห็นเลขจำนวนครั้ง และธง SAFE START
    bool abnormalReset = (lastResetReason == ESP_RST_BROWNOUT || lastResetReason == ESP_RST_TASK_WDT ||
                          lastResetReason == ESP_RST_INT_WDT || lastResetReason == ESP_RST_WDT || lastResetReason == ESP_RST_PANIC);
    char rbuf[28];
    snprintf(rbuf, sizeof(rbuf), safeStart ? "%s SAFE!" : "%s", resetReasonName(lastResetReason));
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.drawString("Last reset:", x, y, 1);
    tft.setTextColor(abnormalReset ? COL_WARN : COL_OK, COL_BG);
    tft.drawString(rbuf, x + 96, y, 1);
    y += lh;
  }
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Free heap:", x, y, 1);
  char heapBuf[16];
  snprintf(heapBuf, sizeof(heapBuf), "%lu KB", (unsigned long)(ESP.getFreeHeap() / 1024));
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(heapBuf, x + 96, y, 1);
  y += lh + 4;

  tft.setTextColor(anyFault ? COL_WARN : COL_OK, COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(anyFault ? "ATTENTION NEEDED" : "ALL SYSTEMS OK", screenW / 2, y, 2);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString("Press to exit", screenW / 2, screenH - 12, 1);
  tft.setTextDatum(TL_DATUM);
}

// วาดไอคอนตามชนิด ที่ตำแหน่ง/ขนาดที่กำหนด (แยกออกมาเพื่อให้เรียกซ้ำได้ระหว่างอนิเมชัน)
void drawIconByType(IconType type, int cx, int cy, int r) {
  switch (type) {
    case ICON_MEASURE: drawIconMeasure(cx, cy, r); break;
    case ICON_CAL: drawIconGear(cx, cy, r); break;
    case ICON_CANCEL: drawIconCross(cx, cy, r); break;
    case ICON_DISTILLED: drawIconDrop(cx, cy, r); break;
    case ICON_SALT: drawIconCrystal(cx, cy, r); break;
    case ICON_DESICCANT: drawIconPouch(cx, cy, r); break;
    case ICON_RECORD: drawIconRecord(cx, cy, r); break;
    case ICON_DELETE: drawIconTrash(cx, cy, r); break;
    case ICON_WIFI: drawIconWifi(cx, cy, r); break;
    case ICON_PREDICT: drawIconPredict(cx, cy, r); break;
    case ICON_COMPARE: drawIconCompare(cx, cy, r); break;
    case ICON_HEALTH: drawIconHealth(cx, cy, r); break;
  }
}

// อนิเมชันไอคอนเมนู: เด้งขยายจากเล็กไปใหญ่ (elastic ease-out) ทุกครั้งที่เปลี่ยนตัวเลือก
void drawCenterIcon(IconType type, const char* label) {
  int cx = screenW / 2, cy = screenH / 2 - 6, r = 34;
  tft.fillRect(0, 24, screenW, screenH - 24, COL_BG);

  const int steps = 8;
  const float stiffness = 1.70158;
  for (int s = 1; s <= steps; s++) {
    float x = (float)s / steps;
    float ee = 1 + (stiffness + 1) * pow(x - 1, 3) + stiffness * pow(x - 1, 2);
    int rr = max(3, (int)(r * ee));
    int clearR = r + 6;
    tft.fillRect(cx - clearR, cy - clearR, clearR * 2, clearR * 2, COL_BG);
    drawIconByType(type, cx, cy, rr);
    delay(12);
  }
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(label, screenW / 2, cy + r + 12, 2);
}

void drawIconMeasure(int cx, int cy, int r) {
  int gr = r * 0.6;
  tft.drawCircle(cx - 5, cy - 5, gr, COL_TEXT);
  tft.drawCircle(cx - 5, cy - 5, gr - 1, COL_TEXT);
  tft.drawLine(cx - 5 + gr * 0.7, cy - 5 + gr * 0.7, cx + r * 0.5, cy + r * 0.5, COL_TEXT);
  for (int i = -2; i <= 2; i++)
    tft.drawLine(cx - 8 + i * 4, cy - 5, cx - 6 + i * 4, cy - 5 + (i % 2 == 0 ? 4 : -4), COL_OK);
}

void drawIconGear(int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r * 0.55, TFT_DARKGREY);
  for (int a = 0; a < 360; a += 45) {
    float rad = a * PI / 180.0;
    int x = cx + cos(rad) * r * 0.8, y = cy + sin(rad) * r * 0.8;
    tft.fillRect(x - 4, y - 4, 8, 8, TFT_DARKGREY);
  }
  tft.fillCircle(cx, cy, r * 0.25, COL_BG);
}

void drawIconCross(int cx, int cy, int r) {
  int s = r * 0.7;
  tft.drawLine(cx - s, cy - s, cx + s, cy + s, COL_WARN);
  tft.drawLine(cx - s + 1, cy - s, cx + s + 1, cy + s, COL_WARN);
  tft.drawLine(cx - s, cy + s, cx + s, cy - s, COL_WARN);
  tft.drawLine(cx - s + 1, cy + s, cx + s + 1, cy - s, COL_WARN);
}

void drawIconDrop(int cx, int cy, int r) {
  tft.fillCircle(cx, cy + r * 0.2, r * 0.6, COL_MILK_BODY);
  tft.fillTriangle(cx, cy - r, cx - r * 0.55, cy, cx + r * 0.55, cy, COL_MILK_BODY);
  tft.fillCircle(cx - r * 0.2, cy, r * 0.15, TFT_WHITE);
}

void drawIconCrystal(int cx, int cy, int r) {
  tft.fillTriangle(cx, cy - r * 0.7, cx - r * 0.6, cy + r * 0.2, cx + r * 0.6, cy + r * 0.2, TFT_WHITE);
  tft.fillTriangle(cx, cy + r * 0.7, cx - r * 0.6, cy + r * 0.2, cx + r * 0.6, cy + r * 0.2, 0xC618);
  tft.drawLine(cx, cy - r * 0.7, cx, cy + r * 0.7, TFT_DARKGREY);
}

void drawIconPouch(int cx, int cy, int r) {
  tft.fillRoundRect(cx - r * 0.55, cy - r * 0.6, r * 1.1, r * 1.3, 4, 0xC618);
  tft.drawRoundRect(cx - r * 0.55, cy - r * 0.6, r * 1.1, r * 1.3, 4, TFT_DARKGREY);
  for (int i = 0; i < 5; i++) tft.fillCircle(cx - r * 0.3 + i * (r * 0.15), cy - r * 0.2 + (i % 2) * 6, 1, TFT_DARKGREY);
}

void drawIconTrash(int cx, int cy, int r) {
  tft.fillRoundRect(cx - r * 0.65, cy - r * 0.75, r * 1.3, r * 0.16, 1, COL_WARN);
  tft.fillRoundRect(cx - r * 0.22, cy - r * 0.95, r * 0.44, r * 0.22, 1, COL_WARN);
  tft.fillRoundRect(cx - r * 0.5, cy - r * 0.55, r * 1.0, r * 1.3, 2, COL_WARN);
  tft.drawRoundRect(cx - r * 0.5, cy - r * 0.55, r * 1.0, r * 1.3, 2, TFT_WHITE);
  for (int i = -1; i <= 1; i++)
    tft.drawFastVLine(cx + i * r * 0.22, cy - r * 0.35, r * 0.9, TFT_WHITE);
}

void drawIconWifi(int cx, int cy, int r) {
  int baseY = cy + r * 0.55;
  tft.fillCircle(cx, baseY, r * 0.14, COL_OK);
  for (int ring = 1; ring <= 3; ring++) {
    float rr = r * (0.32 + ring * 0.24);
    for (int a = 200; a <= 340; a += 3) {
      float rad = a * PI / 180.0;
      int x = cx + cos(rad) * rr;
      int y = baseY + sin(rad) * rr;
      tft.drawPixel(x, y, COL_OK);
      tft.drawPixel(x, y + 1, COL_OK);
    }
  }
}

// ไอคอนหน้า System Health: วงกลม (เหมือนป้าย/สถานะ) + เส้นชีพจร (pulse/ECG) ตรงกลาง สื่อถึง "สุขภาพระบบ"
void drawIconHealth(int cx, int cy, int r) {
  tft.drawCircle(cx, cy, r, COL_OK);
  tft.drawCircle(cx, cy, r - 1, COL_OK);
  int y = cy;
  int x0 = cx - r * 0.7f;
  int seg = r * 0.32f;
  tft.drawLine(x0, y, x0 + seg, y, COL_OK);
  tft.drawLine(x0 + seg, y, x0 + seg * 1.4f, y - r * 0.7f, COL_OK);
  tft.drawLine(x0 + seg * 1.4f, y - r * 0.7f, x0 + seg * 1.8f, y + r * 0.7f, COL_OK);
  tft.drawLine(x0 + seg * 1.8f, y + r * 0.7f, x0 + seg * 2.2f, y, COL_OK);
  tft.drawLine(x0 + seg * 2.2f, y, cx + r * 0.7f, y, COL_OK);
}

// ไอคอนโหมดทำนายค่า: แท่งกราฟ 3 แท่งไล่สูงขึ้น (ค่าที่วัดได้จริงตามเวลา) + เส้นประชี้ไปยัง
// วงกลม (จุดที่ทำนายไว้ล่วงหน้าว่าจะไปถึง) สื่อถึงการ "คาดการณ์ล่วงหน้าจากแนวโน้ม"
void drawIconPredict(int cx, int cy, int r) {
  int barW = r * 0.32;
  float xs[3] = { cx - r * 0.7f, cx - r * 0.1f, cx + r * 0.5f };
  float hs[3] = { r * 0.5f, r * 0.9f, r * 1.3f };
  float baseY = cy + r * 0.7f;
  for (int i = 0; i < 3; i++)
    tft.fillRoundRect(xs[i], baseY - hs[i], barW, hs[i], 1, COL_OK);
  float px = cx + r * 0.75f, py = cy - r * 0.85f;
  float lastX = xs[2] + barW / 2.0f, lastY = baseY - hs[2];
  for (int t = 0; t <= 10; t += 2) {
    float f = t / 10.0f;
    int x = lastX + f * (px - lastX);
    int y = lastY + f * (py - lastY);
    tft.drawPixel(x, y, COL_WARN);
  }
  tft.drawCircle(px, py, r * 0.18, COL_WARN);
}

// ไอคอนโหมดเปรียบเทียบ: แท่ง 2 แท่งสูงต่ำต่างกันคนละสี คั่นด้วยเส้นคู่แนวตั้งตรงกลาง สื่อถึง "A vs B"
void drawIconCompare(int cx, int cy, int r) {
  int barW = r * 0.35f;
  float hA = r * 0.9f, hB = r * 1.3f;
  float xA = cx - r * 0.65f, xB = cx + r * 0.3f;
  float baseY = cy + r * 0.7f;
  tft.fillRoundRect(xA, baseY - hA, barW, hA, 1, COL_TEMP);
  tft.fillRoundRect(xB, baseY - hB, barW, hB, 1, COL_OK);
  tft.drawFastVLine(cx - 1, cy - r * 0.9f, r * 1.6f, COL_TEXT);
  tft.drawFastVLine(cx + 1, cy - r * 0.9f, r * 1.6f, COL_TEXT);
}

void drawIconRecord(int cx, int cy, int r) {
  tft.fillRoundRect(cx - r * 0.7, cy - r * 0.8, r * 1.4, r * 1.6, 3, TFT_DARKGREY);
  tft.drawRoundRect(cx - r * 0.7, cy - r * 0.8, r * 1.4, r * 1.6, 3, TFT_WHITE);
  tft.fillRect(cx - r * 0.4, cy - r * 0.8, r * 0.8, r * 0.5, TFT_WHITE);
  tft.fillCircle(cx, cy + r * 0.35, r * 0.28, TFT_BLACK);
  tft.drawCircle(cx, cy + r * 0.35, r * 0.28, TFT_WHITE);
}

void triggerIcon(float atValue, int type) {
  iconVisible = true;
  activeIconType = type;
  iconShownAt = millis();
  int touchY = valueToY(atValue);
  iconX = constrain(graphX + graphW - 1, graphX + ICON_MAX_RADIUS, graphX + graphW - 1 - ICON_MAX_RADIUS);
  iconY = constrain(touchY - ICON_BASE_RADIUS - 4, graphY + ICON_MAX_RADIUS, graphY + graphH - 1 - ICON_MAX_RADIUS);
}

int getFoodCategoryIndex(float v) {
  for (int i = 0; i < NUM_CATEGORIES; i++)
    if (v >= foodCategories[i].minVal && v < foodCategories[i].maxVal) return i;
  return NUM_CATEGORIES - 1;
}

void clearFoodCategoryLabel() {
  int labelY = graphY + graphH + 2;
  tft.fillRect(0, labelY, screenW, screenH - labelY, COL_BG);
}

void drawFoodCategory(int idx) {
  int labelY = graphY + graphH + 2;
  tft.fillRect(0, labelY, screenW, screenH - labelY, COL_BG);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(foodCategories[idx].color, COL_BG);
  tft.drawString(foodCategories[idx].label, screenW / 2, labelY, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
}

// v12: แทนที่บรรทัดหมวดอาหารบนจอ TFT ด้วยเวลาที่ใช้วัดอยู่ ระหว่างกำลังวัดค่าจริง (วัดปกติ/ทำนาย/เปรียบเทียบ)
// ใช้ตำแหน่งเดียวกับ drawFoodCategory() เดิม และทำเหมือนที่ทำกับจอ LCD ไปแล้ว (ดู formatElapsedTimeLCD)
// หมวดอาหารยังคงแสดงอยู่เหมือนเดิมในหน้าผลลัพธ์ที่วัดเสร็จแล้ว (drawRecordViewScreen)
// เพราะตรงนั้นคือคำตอบสุดท้าย ไม่ใช่หน้าจอที่กำลังวัดอยู่
void drawElapsedTimeTFT(unsigned long startMs) {
  int labelY = graphY + graphH + 2;
  tft.fillRect(0, labelY, screenW, screenH - labelY, COL_BG);
  unsigned long elapsedSec = (millis() - startMs) / 1000UL;
  unsigned int mm = (unsigned int)(elapsedSec / 60UL);
  unsigned int ss = (unsigned int)(elapsedSec % 60UL);
  if (mm > 99) mm = 99;
  char buf[16];
  snprintf(buf, sizeof(buf), "Time %02u:%02u", mm, ss);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(buf, screenW / 2, labelY, 1);
  tft.setTextDatum(TL_DATUM);
}

// v14: แทนที่บรรทัดหมวดอาหารบนจอ TFT ด้วยเวลาที่ใช้วัด "แบบค่าคงที่ที่จับไว้แล้ว" ในหน้าผลลัพธ์/บันทึก
// (หน้าถามเซฟหลังวัดเสร็จ และหน้าดูรายการที่บันทึกไว้) ใช้ตำแหน่งเดียวกับ drawFoodCategory() เดิม
void drawFinalDurationTFT(unsigned long durationSec) {
  int labelY = graphY + graphH + 2;
  tft.fillRect(0, labelY, screenW, screenH - labelY, COL_BG);
  char buf[16];
  formatDurationShort(durationSec, buf, sizeof(buf));
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(buf, screenW / 2, labelY, 1);
  tft.setTextDatum(TL_DATUM);
}

void updateLCD(float v, int holdPhase, int idx) {
  lcd.setCursor(0, 0);
  char tbuf[8];
  formatTempC(tbuf, sizeof(tbuf));
  char statCh = (holdPhase == 2) ? 'H' : (holdPhase == 1) ? 'Y' : '-';
  char l1[17];
  snprintf(l1, sizeof(l1), "aw:%.3f %c%s", v, statCh, tbuf);
  lcd.print(l1);
  lcd.setCursor(0, 1);
  // v11: ตัดบรรทัดหมวดอาหารออกจากจอ LCD (ยังคงโชว์บนจอ TFT ตามเดิมผ่าน drawFoodCategory())
  // เปลี่ยนบรรทัดนี้เป็นระยะเวลาที่กำลังวัดอยู่แทน นับจากตอนเริ่มวัด (measureStartMs)
  (void)idx; // ไม่ใช้หมวดอาหารในบรรทัด LCD นี้แล้ว แต่ยังรับพารามิเตอร์ไว้เผื่อใช้งานอื่นในอนาคต
  char l2[17];
  formatElapsedTimeLCD(measureStartMs, l2, sizeof(l2));
  String lbl = String(l2);
  while (lbl.length() < 16) lbl += ' ';
  lcd.print(lbl.substring(0, 16));
}

// v12: กรองสัญญาณแบบ EMA (Exponential Moving Average) ก่อนเก็บลงกราฟ เพื่อลดการแกว่ง/สั่นไหว
// เล็ก ๆ ที่เกิดจาก noise ของเซ็นเซอร์ โดยตั้งค่า alpha ไว้ไม่ต่ำเกินไป เพื่อไม่ให้กราฟ "หน่วง/ล้าช้า"
// ตามแนวโน้มค่าจริงมากนัก (alpha ยิ่งน้อย = กรองแรง/นิ่งขึ้น แต่ตามค่าจริงช้าลง)
// เลือก 0.35 เป็นจุดกึ่งกลาง: ตัดจุดสั่นไหวเล็ก ๆ ของเซนเซอร์ออกได้ชัดเจน แต่ยังไล่ตามแนวโน้มค่าจริงทันใน 2-3 วินาที
const float GRAPH_EMA_ALPHA = 0.35;
float graphEmaAw = NAN;
float graphEmaTemp = NAN;

// เรียกคู่กับ numPoints = 0 ทุกครั้งที่เริ่มวัดตัวอย่างใหม่ กันไม่ให้ค่า EMA ของตัวอย่างก่อนหน้า
// มาถ่วงจุดแรกของกราฟตัวอย่างใหม่ (ไม่งั้นจุดแรก ๆ ของกราฟใหม่จะยังค่อย ๆ ไล่มาจากค่าเก่า)
void resetGraphFilter() {
  graphEmaAw = NAN;
  graphEmaTemp = NAN;
}

void pushValue(float v, float t) {
  // จุดแรกของแต่ละรอบวัด (EMA ยังไม่มีค่าเก่า) ใช้ค่าจริงตรง ๆ ไปเลย ไม่ต้องรอ "อุ่นเครื่อง" ตัวกรอง
  graphEmaAw = isnan(graphEmaAw) ? v : (GRAPH_EMA_ALPHA * v + (1.0f - GRAPH_EMA_ALPHA) * graphEmaAw);
  float vf = graphEmaAw;

  float tf = t;
  if (!isnan(t)) {
    graphEmaTemp = isnan(graphEmaTemp) ? t : (GRAPH_EMA_ALPHA * t + (1.0f - GRAPH_EMA_ALPHA) * graphEmaTemp);
    tf = graphEmaTemp;
  }

  if (numPoints < graphW) {
    values[numPoints] = vf;
    tempValues[numPoints] = tf;
    numPoints++;
  } else {
    for (int i = 0; i < graphW - 1; i++) {
      values[i] = values[i + 1];
      tempValues[i] = tempValues[i + 1];
    }
    values[graphW - 1] = vf;
    tempValues[graphW - 1] = tf;
  }
}

// สไตล์ตามกราฟเว็บ: เส้นทึบ = Aw, เส้นประ = อุณหภูมิ (สีต่างกันแทนแกนขวา)
void drawGraphLegend() {
  int ly = graphY - 12;
  tft.drawFastHLine(4, ly, 10, COL_LINE);
  tft.setTextColor(COL_LINE, COL_BG);
  tft.drawString("Aw", 16, ly - 4, 1);
  int tx = 46;
  for (int x = tx; x < tx + 10; x += 3) tft.drawPixel(x, ly, COL_TEMP);
  tft.setTextColor(COL_TEMP, COL_BG);
  tft.drawString("Temp", tx + 12, ly - 4, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
}

void drawGraphFrame() {
  tft.drawRect(graphX - 1, graphY - 1, graphW + 2, graphH + 2, COL_AXIS);
  int yTop = graphY, yMid = graphY + graphH / 2, yBottom = graphY + graphH;
  tft.drawFastHLine(graphX, yTop, graphW, COL_GRID);
  tft.drawFastHLine(graphX, yMid, graphW, COL_GRID);
  tft.drawFastHLine(graphX, yBottom, graphW, COL_GRID);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(MR_DATUM);
  tft.drawString("1.0", graphX - 4, yTop, 1);
  tft.drawString("0.5", graphX - 4, yMid, 1);
  tft.drawString("0.0", graphX - 4, yBottom, 1);
  tft.setTextDatum(TL_DATUM);
  drawGraphLegend();
}

void drawStaticUI() {
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("Water Activity (aw)", 4, 4, 2);
  drawGraphFrame();
}

void drawGraph(int holdPhase) {
  tft.fillRect(graphX, graphY, graphW, graphH, COL_BG);
  tft.drawFastHLine(graphX, graphY, graphW, COL_GRID);
  tft.drawFastHLine(graphX, graphY + graphH / 2, graphW, COL_GRID);
  tft.drawFastHLine(graphX, graphY + graphH - 1, graphW, COL_GRID);
  if (holdPhase >= 1) {
    int yTouchLine = valueToY(currentAw);
    uint16_t dashColor = (holdPhase == 1) ? COL_TOUCH : (holdPhase == 2) ? COL_OK : COL_WARN;
    for (int x = graphX; x < graphX + graphW; x += 4) tft.drawPixel(x, yTouchLine, dashColor);
  }
  uint16_t lineColor = (holdPhase == 1) ? COL_TOUCH : (holdPhase == 2) ? COL_OK : (holdPhase == 3) ? COL_WARN : COL_LINE;
  for (int i = 0; i < numPoints - 1; i++)
    tft.drawLine(graphX + i, valueToY(values[i]), graphX + i + 1, valueToY(values[i + 1]), lineColor);
  // เส้นประอุณหภูมิ ซ้อนคู่กับเส้น Aw (คนละสี = คนละแกน เหมือนกราฟบนเว็บ)
  for (int i = 0; i < numPoints - 1; i++) {
    if ((i % 4) < 2)
      tft.drawLine(graphX + i, valueToYTemp(tempValues[i]), graphX + i + 1, valueToYTemp(tempValues[i + 1]), COL_TEMP);
  }
}

int valueToY(float v) {
  return graphY + (int)((1.0 - v) * (graphH - 1));
}

const float TEMP_TFT_MIN = 0.0;  // สเกลอุณหภูมิบนจอ TFT (°C) ให้ตรงกับกราฟบนเว็บ (0-60°C)
const float TEMP_TFT_MAX = 60.0;
int valueToYTemp(float t) {
  if (isnan(t)) t = TEMP_TFT_MIN;
  float frac = (t - TEMP_TFT_MIN) / (TEMP_TFT_MAX - TEMP_TFT_MIN);
  if (frac < 0) frac = 0;
  if (frac > 1) frac = 1;
  return graphY + (int)((1.0 - frac) * (graphH - 1));
}

void drawCurrentTemp(float t) {
  char buf[10];
  bool warn = isTempOutOfCalRange(t) || peltierStuckHot;
  if (isnan(t)) snprintf(buf, sizeof(buf), "--.-C");
  else snprintf(buf, sizeof(buf), warn ? "%.1fC!" : "%.1fC", t);
  tft.setTextDatum(TR_DATUM);
  tft.fillRect(screenW - 60, 20, 58, 14, COL_BG);
  tft.setTextColor(warn ? COL_WARN : COL_TEMP, COL_BG);
  tft.drawString(buf, screenW - 4, 22, 1);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
}

void drawCurrentValue(float v, int holdPhase) {
  uint16_t color = (v > 0.85 || v < 0.20) ? COL_WARN : COL_OK;
  if (holdPhase == 1) color = COL_TOUCH;
  else if (holdPhase == 2) color = COL_OK;
  else if (holdPhase == 3) color = COL_WARN;
  tft.setTextDatum(TR_DATUM);
  tft.fillRect(screenW - 68, 2, 66, 16, COL_BG);  // ขยายกล่องเล็กน้อยให้พอดีกับทศนิยมที่เพิ่มเป็น 3 ตำแหน่ง
  tft.setTextColor(color, COL_BG);
  tft.drawFloat(v, 3, screenW - 4, 4, 2);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.setTextDatum(TL_DATUM);
}

void drawActiveIcon(int cx, int cy, unsigned long age) {
  int radius;
  int yOff = sin(age * 0.01) * 2;
  if (age < 220) {
    float x = age / 220.0, s = 1.70158;
    float ee = 1 + (s + 1) * pow(x - 1, 3) + s * pow(x - 1, 2);
    radius = max(2, (int)(ICON_BASE_RADIUS * ee));
  } else radius = ICON_BASE_RADIUS;
  tft.fillEllipse(cx, cy + radius + 4 + yOff, radius, radius / 3, TFT_DARKGREY);
  cy += yOff;
  if (activeIconType == 0) drawCookieIcon(cx, cy, radius);
  else if (activeIconType == 1) drawFruitIcon(cx, cy, radius);
  else if (activeIconType == 2) drawMeatIcon(cx, cy, radius);
  else drawMilkIcon(cx, cy, radius);
}

void drawCookieIcon(int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r, COL_COOKIE_BODY);
  tft.fillCircle(cx - 2, cy - 2, r / 3, TFT_WHITE);
  tft.drawCircle(cx, cy, r, TFT_BLACK);
  if (r >= ICON_BASE_RADIUS - 1) {
    tft.fillCircle(cx - 3, cy - 2, 1, COL_COOKIE_CHIP);
    tft.fillCircle(cx + 2, cy - 1, 1, COL_COOKIE_CHIP);
    tft.fillCircle(cx - 1, cy + 2, 1, COL_COOKIE_CHIP);
    tft.fillCircle(cx + 3, cy + 2, 1, COL_COOKIE_CHIP);
  }
}

void drawFruitIcon(int cx, int cy, int r) {
  tft.fillCircle(cx, cy, r, COL_FRUIT_BODY);
  tft.drawCircle(cx, cy, r, TFT_BLACK);
  if (r >= ICON_BASE_RADIUS - 1) {
    tft.drawLine(cx, cy - r, cx + 2, cy - r - 2, COL_FRUIT_LEAF);
    tft.drawLine(cx, cy - r, cx - 1, cy - r - 3, 0x7BEF);
  }
}

void drawMeatIcon(int cx, int cy, int r) {
  int w = r * 2, h = r * 1.5;
  tft.fillRoundRect(cx - r, cy - r + 2, w, h, 2, COL_MEAT_BODY);
  tft.drawRoundRect(cx - r, cy - r + 2, w, h, 2, TFT_BLACK);
  if (r >= ICON_BASE_RADIUS - 1) {
    tft.drawLine(cx - r + 2, cy, cx + r - 2, cy, COL_MEAT_FAT);
    tft.drawLine(cx - r + 4, cy + 2, cx + r - 4, cy + 2, COL_MEAT_FAT);
  }
}

void drawMilkIcon(int cx, int cy, int r) {
  int w = r * 1.5, h = r * 2;
  tft.fillRect(cx - w / 2, cy - r, w, h, TFT_WHITE);
  tft.drawRect(cx - w / 2, cy - r, w, h, TFT_BLACK);
  if (r >= ICON_BASE_RADIUS - 1) {
    tft.fillRect(cx - w / 2, cy - r, w, h / 3, COL_MILK_BODY);
    tft.drawLine(cx - w / 2 + 2, cy + 2, cx + w / 2 - 2, cy + 2, COL_MILK_BODY);
  }
}

// =====================================================================================
// มาสคอต "RuiBo": หุ่นยนต์หัวจอสี่เหลี่ยมมุมมน ตาเหลือง 2 ดวง ปากยิ้ม เสาอากาศ+ใบหูด้านข้าง
// ใช้แทนไอคอนหน้าจอเปิดเครื่อง (splash) และหน้า MAIN MENU โดยเฉพาะ
// ปรับสี/สัดส่วนได้ตรงนี้ที่เดียว ไม่กระทบไอคอนเมนูย่อยเดิม
// =====================================================================================
#define COL_MASCOT_BODY TFT_DARKGREY
#define COL_MASCOT_SCREEN TFT_BLACK
#define COL_MASCOT_EYE TFT_YELLOW

// วาดหน้ามาสคอตที่ตำแหน่ง (cx,cy) รัศมีอ้างอิง r, eyesOpen=false คือกำลังกระพริบตา (ตาปิด)
void drawMascotFace(int cx, int cy, int r, bool eyesOpen) {
  int headW = (int)(r * 1.9);
  int headH = (int)(r * 1.6);
  int hx = cx - headW / 2;
  int hy = cy - headH / 2;
  int corner = max(3, r / 3);

  // เสาอากาศเล็ก ๆ บนหัว 2 เส้น
  int antTopY = hy - (int)(r * 0.35);
  int antLX = cx - (int)(headW * 0.22);
  int antRX = cx + (int)(headW * 0.22);
  tft.drawLine(antLX, hy, antLX, antTopY, COL_MASCOT_BODY);
  tft.drawLine(antRX, hy, antRX, antTopY, COL_MASCOT_BODY);
  tft.fillCircle(antLX, antTopY, max(2, r / 10), COL_MASCOT_BODY);
  tft.fillCircle(antRX, antTopY, max(2, r / 10), COL_MASCOT_BODY);

  // ใบหูทรงกลมด้านข้างหัว
  tft.fillCircle(hx, cy, max(3, (int)(r * 0.22)), COL_MASCOT_BODY);
  tft.fillCircle(hx + headW, cy, max(3, (int)(r * 0.22)), COL_MASCOT_BODY);

  // ตัวเคสหัว (โค้งมน)
  tft.fillRoundRect(hx, hy, headW, headH, corner, COL_MASCOT_BODY);
  tft.drawRoundRect(hx, hy, headW, headH, corner, TFT_BLACK);

  // จอหน้า (สีดำ) ฝังกลางหัว
  int pad = max(2, r / 6);
  int scrW = headW - pad * 2;
  int scrH = (int)(headH * 0.72);
  int scrX = hx + pad;
  int scrY = hy + pad;
  tft.fillRoundRect(scrX, scrY, scrW, scrH, max(2, corner / 2), COL_MASCOT_SCREEN);

  // ดวงตา
  int eyeY = scrY + (int)(scrH * 0.42);
  int eyeOffX = (int)(scrW * 0.24);
  int eyeR = max(2, (int)(r * 0.16));
  if (eyesOpen) {
    tft.fillCircle(cx - eyeOffX, eyeY, eyeR, COL_MASCOT_EYE);
    tft.fillCircle(cx + eyeOffX, eyeY, eyeR, COL_MASCOT_EYE);
  } else {
    tft.fillRect(cx - eyeOffX - eyeR, eyeY - 1, eyeR * 2, 2, COL_MASCOT_EYE);
    tft.fillRect(cx + eyeOffX - eyeR, eyeY - 1, eyeR * 2, 2, COL_MASCOT_EYE);
  }

  // ปากยิ้มเล็ก ๆ ใต้ดวงตา
  int mouthY = scrY + (int)(scrH * 0.72);
  int mouthW = (int)(scrW * 0.30);
  tft.drawLine(cx - mouthW / 2, mouthY, cx, mouthY + 3, COL_MASCOT_EYE);
  tft.drawLine(cx, mouthY + 3, cx + mouthW / 2, mouthY, COL_MASCOT_EYE);
}

// ตัวแปรสถานะมาสคอตหน้า MAIN MENU (เก็บตำแหน่ง/ขนาดล่าสุดไว้ เพื่อวาดซ้ำตอนกระพริบตาแบบ partial redraw)
int mascotCX = 0, mascotCY = 0, mascotR = 30;
bool mascotEyesOpen = true;
unsigned long mascotLastBlinkMs = 0;
const unsigned long MASCOT_BLINK_EVERY_MS = 2800;  // ทุกกี่ ms จะกระพริบตาหนึ่งครั้ง
const unsigned long MASCOT_BLINK_HOLD_MS = 150;    // หลับตานานกี่ ms ต่อครั้ง

// วาดมาสคอตกลางจอสำหรับหน้า MAIN MENU พร้อมอนิเมชันเด้งโผล่ (elastic ease-out เหมือนไอคอนเมนูอื่น)
void drawMascotCenter(const char* label) {
  mascotCX = screenW / 2;
  mascotCY = screenH / 2 - 6;
  mascotR = min(screenW, screenH) / 7;
  if (mascotR < 16) mascotR = 16;

  tft.fillRect(0, 24, screenW, screenH - 24, COL_BG);

  const int steps = 8;
  const float stiffness = 1.70158;
  for (int s = 1; s <= steps; s++) {
    float x = (float)s / steps;
    float ee = 1 + (stiffness + 1) * pow(x - 1, 3) + stiffness * pow(x - 1, 2);
    int rr = max(4, (int)(mascotR * ee));
    int clearR = mascotR + 10;
    tft.fillRect(mascotCX - clearR, mascotCY - clearR, clearR * 2, clearR * 2, COL_BG);
    drawMascotFace(mascotCX, mascotCY, rr, true);
    delay(12);
  }
  mascotEyesOpen = true;
  mascotLastBlinkMs = millis();

  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(COL_TEXT, COL_BG);
  tft.drawString(label, screenW / 2, mascotCY + mascotR + 16, 2);
  tft.setTextDatum(TL_DATUM);
}

// เรียกทุกรอบ loop() ตอนอยู่หน้า MAIN MENU เพื่อให้มาสคอตกระพริบตาเป็นระยะ (วาดจริงเฉพาะตอนถึงจังหวะ)
void updateMascotIdleBlink() {
  unsigned long now = millis();
  if (mascotEyesOpen) {
    if (now - mascotLastBlinkMs >= MASCOT_BLINK_EVERY_MS) {
      mascotEyesOpen = false;
      mascotLastBlinkMs = now;
      drawMascotFace(mascotCX, mascotCY, mascotR, false);
    }
  } else {
    if (now - mascotLastBlinkMs >= MASCOT_BLINK_HOLD_MS) {
      mascotEyesOpen = true;
      mascotLastBlinkMs = now;
      drawMascotFace(mascotCX, mascotCY, mascotR, true);
    }
  }
}

// อนิเมชันเด้งโผล่ของมาสคอตตอนเปิดเครื่อง (splash screen) ก่อนเข้าสู่ลูปกระพริบตาระหว่างโหลด
void drawMascotSplashEntrance(int cx, int cy, int r) {
  const int steps = 10;
  const float stiffness = 1.70158;
  for (int s = 1; s <= steps; s++) {
    float x = (float)s / steps;
    float ee = 1 + (stiffness + 1) * pow(x - 1, 3) + stiffness * pow(x - 1, 2);
    int rr = max(4, (int)(r * ee));
    int clearR = r + 14;
    tft.fillRect(cx - clearR, cy - clearR, clearR * 2, clearR * 2, TFT_BLACK);
    drawMascotFace(cx, cy, rr, true);
    delay(14);
  }
}
