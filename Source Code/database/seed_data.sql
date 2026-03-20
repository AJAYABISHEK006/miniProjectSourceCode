-- ============================================================
-- NexusBank Seed Data
-- Pre-loaded fake users for testing and demo
-- Group A: NetBanking already active (login directly)
-- Group B: Bank customers, need to activate NetBanking first
-- ============================================================

USE nexusbank;

-- ========================
-- GROUP A USERS (Active)
-- ========================

-- User 1: Arjun Sharma, New Delhi
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100001',
    'Arjun Sharma',
    '1992-08-15',
    'Male',
    'ABCPA1234D',
    '234567890123',
    '9876543210',
    'arjun.sharma@gmail.com',
    '42 Rajpur Road, Vasant Kunj',
    'New Delhi',
    'Delhi',
    '110070',
    -- password: arjun@123 (bcrypt hash generated at runtime by Rust seeder)
    '$2b$12$arjun_placeholder_hash_here_replace_with_real',
    1
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100001', '50100234567891', 'Savings', 'NXB0001001', 'Vasant Kunj New Delhi', 428750.00, '2019-03-12');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100001', '4111223344556601', '08/27', '$2b$12$cvv_452_hash_placeholder', 'Platinum Debit', 'Active');

INSERT INTO transactions (customer_id, txn_time, description, txn_type, amount, bal_after, ref_id)
VALUES
('NXB100001', '2024-03-15 10:30:00', 'Amazon Purchase',        'Debit',  2499.00,  426251.00, 'TXN20240315001'),
('NXB100001', '2024-03-12 09:00:00', 'Salary Credit',          'Credit', 85000.00, 511251.00, 'TXN20240312001'),
('NXB100001', '2024-03-10 14:20:00', 'Electricity Bill',       'Debit',  1240.00,  426251.00, 'TXN20240310001'),
('NXB100001', '2024-03-08 11:45:00', 'NEFT to Raj Kumar',      'Debit',  15000.00, 427491.00, 'TXN20240308001'),
('NXB100001', '2024-03-05 19:10:00', 'Swiggy Order',           'Debit',  450.00,   442491.00, 'TXN20240305001'),
('NXB100001', '2024-02-28 16:00:00', 'ATM Withdrawal',         'Debit',  10000.00, 442941.00, 'TXN20240228001'),
('NXB100001', '2024-02-25 13:30:00', 'UPI to Priya',           'Debit',  5000.00,  452941.00, 'TXN20240225001'),
('NXB100001', '2024-02-20 09:00:00', 'Salary Credit',          'Credit', 85000.00, 457941.00, 'TXN20240220001'),
('NXB100001', '2024-02-15 20:00:00', 'Netflix Subscription',   'Debit',  649.00,   372941.00, 'TXN20240215001'),
('NXB100001', '2024-02-10 08:00:00', 'Rent Transfer',          'Debit',  25000.00, 373590.00, 'TXN20240210001');


-- User 2: Priya Nair, Chennai
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100002',
    'Priya Nair',
    '1995-03-22',
    'Female',
    'BCDPN5678E',
    '345678901234',
    '9876543211',
    'priya.nair@gmail.com',
    '15 Anna Salai, T Nagar',
    'Chennai',
    'Tamil Nadu',
    '600017',
    -- password: priya@123
    '$2b$12$priya_placeholder_hash_here_replace_with_real',
    1
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100002', '50100234567892', 'Salary', 'NXB0002001', 'T Nagar Chennai', 185500.00, '2020-06-18');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100002', '4111223344556602', '06/26', '$2b$12$cvv_321_hash_placeholder', 'Classic Debit', 'Active');

INSERT INTO transactions (customer_id, txn_time, description, txn_type, amount, bal_after, ref_id)
VALUES
('NXB100002', '2024-03-14 09:05:00', 'Salary Credit',      'Credit', 55000.00, 188999.00, 'TXN20240314002'),
('NXB100002', '2024-03-12 15:40:00', 'Flipkart Purchase',  'Debit',  3299.00,  133999.00, 'TXN20240312002'),
('NXB100002', '2024-03-10 11:00:00', 'Water Bill',         'Debit',  450.00,   137298.00, 'TXN20240310002'),
('NXB100002', '2024-03-07 10:30:00', 'NEFT to Mother',     'Debit',  10000.00, 137748.00, 'TXN20240307002'),
('NXB100002', '2024-03-03 20:15:00', 'Zomato Order',       'Debit',  380.00,   147748.00, 'TXN20240303002'),
('NXB100002', '2024-02-28 17:00:00', 'ATM Withdrawal',     'Debit',  5000.00,  148128.00, 'TXN20240228002'),
('NXB100002', '2024-02-25 14:20:00', 'UPI to Friend',      'Debit',  2000.00,  153128.00, 'TXN20240225002'),
('NXB100002', '2024-02-14 09:00:00', 'Salary Credit',      'Credit', 55000.00, 155128.00, 'TXN20240214002'),
('NXB100002', '2024-02-10 22:00:00', 'Amazon Prime',       'Debit',  1499.00,  100128.00, 'TXN20240210002'),
('NXB100002', '2024-02-05 12:30:00', 'Medical Bills',      'Debit',  3200.00,  101627.00, 'TXN20240205002');


-- User 3: Ravi Kumar, Mumbai
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100003',
    'Ravi Kumar',
    '1988-11-10',
    'Male',
    'CDERK9012F',
    '456789012345',
    '9876543212',
    'ravi.kumar@gmail.com',
    '8 MG Road, Bandra West',
    'Mumbai',
    'Maharashtra',
    '400050',
    -- password: ravi@123
    '$2b$12$ravi_placeholder_hash_here_replace_with_real',
    1
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100003', '50100234567893', 'Savings', 'NXB0003001', 'Bandra Mumbai', 92300.00, '2017-09-05');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100003', '4111223344556603', '03/25', '$2b$12$cvv_789_hash_placeholder', 'Classic Debit', 'Active');

INSERT INTO transactions (customer_id, txn_time, description, txn_type, amount, bal_after, ref_id)
VALUES
('NXB100003', '2024-03-13 10:00:00', 'Business Income',    'Credit', 30000.00, 95300.00,  'TXN20240313003'),
('NXB100003', '2024-03-11 18:30:00', 'Grocery Store',      'Debit',  2100.00,  65300.00,  'TXN20240311003'),
('NXB100003', '2024-03-09 12:00:00', 'Phone Bill',         'Debit',  799.00,   67400.00,  'TXN20240309003'),
('NXB100003', '2024-03-06 09:00:00', 'NEFT to Partner',    'Debit',  20000.00, 68199.00,  'TXN20240306003'),
('NXB100003', '2024-03-02 16:00:00', 'Petrol',             'Debit',  3000.00,  88199.00,  'TXN20240302003'),
('NXB100003', '2024-02-27 11:00:00', 'ATM Withdrawal',     'Debit',  8000.00,  91199.00,  'TXN20240227003'),
('NXB100003', '2024-02-22 14:00:00', 'Insurance Premium',  'Debit',  5500.00,  99199.00,  'TXN20240222003'),
('NXB100003', '2024-02-18 10:00:00', 'Business Income',    'Credit', 30000.00, 104699.00, 'TXN20240218003'),
('NXB100003', '2024-02-12 19:00:00', 'Electricity Bill',   'Debit',  1800.00,  74699.00,  'TXN20240212003'),
('NXB100003', '2024-02-08 08:00:00', 'Car EMI',            'Debit',  12000.00, 76499.00,  'TXN20240208003');


-- ========================
-- GROUP B USERS (Inactive - need to activate NetBanking)
-- ========================

-- User 4: Karthik Raja, Chennai
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100006',
    'Karthik Raja',
    '1991-04-28',
    'Male',
    'FGHKR2345I',
    '789012345678',
    '9876543215',
    'karthik.raja@gmail.com',
    '12 Nungambakkam High Road',
    'Chennai',
    'Tamil Nadu',
    '600034',
    NULL,   -- no password set yet
    0       -- netbanking not active
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100006', '50100234567896', 'Savings', 'NXB0006001', 'Nungambakkam Chennai', 245000.00, '2021-01-15');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100006', '4111223344556606', '04/26', '$2b$12$cvv_987_hash_placeholder', 'Classic Debit', 'Active');


-- User 5: Deepa Menon, Kochi
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100007',
    'Deepa Menon',
    '1994-09-14',
    'Female',
    'GHIDM4567J',
    '890123456789',
    '9876543216',
    'deepa.menon@gmail.com',
    '7 MG Road, Ernakulam',
    'Kochi',
    'Kerala',
    '682016',
    NULL,
    0
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100007', '50100234567897', 'Salary', 'NXB0007001', 'Ernakulam Kochi', 88500.00, '2022-03-10');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100007', '4111223344556607', '07/25', '$2b$12$cvv_246_hash_placeholder', 'Classic Debit', 'Active');


-- User 6: Suresh Babu, Bangalore
INSERT INTO users (customer_id, full_name, dob, gender, pan_number, aadhaar, mobile, email, house_addr, city, state_name, pin_code, pwd_hash, net_active)
VALUES (
    'NXB100008',
    'Suresh Babu',
    '1987-06-30',
    'Male',
    'HIJSB6789K',
    '901234567890',
    '9876543217',
    'suresh.babu@gmail.com',
    '34 Koramangala 6th Block',
    'Bangalore',
    'Karnataka',
    '560095',
    NULL,
    0
);

INSERT INTO accounts (customer_id, acc_number, acc_type, ifsc_code, branch_name, balance, opened_on)
VALUES ('NXB100008', '50100234567898', 'Savings', 'NXB0008001', 'Koramangala Bangalore', 155750.00, '2018-11-20');

INSERT INTO debit_cards (customer_id, card_number, card_expiry, cvv_hash, card_type, card_status)
VALUES ('NXB100008', '4111223344556608', '12/26', '$2b$12$cvv_135_hash_placeholder', 'Classic Debit', 'Active');
