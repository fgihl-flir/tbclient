#include <gtest/gtest.h>
#include "thermal/temperature_reading.h"

namespace thermal {

class TemperatureReadingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test data
        test_timestamp = std::chrono::system_clock::now();
    }
    
    std::chrono::time_point<std::chrono::system_clock> test_timestamp;
};

TEST_F(TemperatureReadingTest, ValidTemperatureReading) {
    TemperatureReading reading;
    reading.spot_id = 1;
    reading.temperature = 25.5;
    reading.timestamp = test_timestamp;
    reading.quality = ReadingQuality::GOOD;
    
    EXPECT_TRUE(reading.validate());
    EXPECT_EQ(reading.spot_id, 1);
    EXPECT_DOUBLE_EQ(reading.temperature, 25.5);
    EXPECT_EQ(reading.quality, ReadingQuality::GOOD);
}

TEST_F(TemperatureReadingTest, InvalidSpotId) {
    TemperatureReading reading;
    reading.spot_id = 0;  // Invalid spot ID
    reading.temperature = 25.5;
    reading.timestamp = test_timestamp;
    reading.quality = ReadingQuality::GOOD;
    
    EXPECT_THROW(reading.validate(), std::invalid_argument);
}

TEST_F(TemperatureReadingTest, TemperatureOutOfRange) {
    TemperatureReading reading1;
    reading1.spot_id = 1;
    reading1.temperature = -150.0;  // Too cold
    reading1.timestamp = test_timestamp;
    reading1.quality = ReadingQuality::GOOD;
    
    EXPECT_THROW(reading1.validate(), std::invalid_argument);
    
    TemperatureReading reading2;
    reading2.spot_id = 1;
    reading2.temperature = 600.0;  // Too hot
    reading2.timestamp = test_timestamp;
    reading2.quality = ReadingQuality::GOOD;
    
    EXPECT_THROW(reading2.validate(), std::invalid_argument);
}

TEST_F(TemperatureReadingTest, BoundaryTemperatures) {
    TemperatureReading reading1;
    reading1.spot_id = 1;
    reading1.temperature = -100.0;  // Minimum valid
    reading1.timestamp = test_timestamp;
    reading1.quality = ReadingQuality::GOOD;
    
    EXPECT_TRUE(reading1.validate());
    
    TemperatureReading reading2;
    reading2.spot_id = 1;
    reading2.temperature = 500.0;  // Maximum valid
    reading2.timestamp = test_timestamp;
    reading2.quality = ReadingQuality::GOOD;
    
    EXPECT_TRUE(reading2.validate());
}

TEST_F(TemperatureReadingTest, FutureTimestamp) {
    TemperatureReading reading;
    reading.spot_id = 1;
    reading.temperature = 25.5;
    reading.timestamp = std::chrono::system_clock::now() + std::chrono::hours(1);  // Future
    reading.quality = ReadingQuality::GOOD;
    
    EXPECT_THROW(reading.validate(), std::invalid_argument);
}

TEST_F(TemperatureReadingTest, ErrorQualityRequiresErrorCode) {
    TemperatureReading reading;
    reading.spot_id = 1;
    reading.temperature = 25.5;
    reading.timestamp = test_timestamp;
    reading.quality = ReadingQuality::ERROR;
    // error_code not set
    
    EXPECT_THROW(reading.validate(), std::invalid_argument);
    
    // Now set error code
    reading.error_code = 1001;
    EXPECT_TRUE(reading.validate());
}

TEST_F(TemperatureReadingTest, JSONSerialization) {
    TemperatureReading reading;
    reading.spot_id = 1;
    reading.temperature = 25.5;
    reading.timestamp = test_timestamp;
    reading.quality = ReadingQuality::GOOD;
    
    auto json = reading.to_json();
    EXPECT_EQ(json["spot_id"], 1);
    EXPECT_DOUBLE_EQ(json["temperature"], 25.5);
    EXPECT_EQ(json["quality"], "GOOD");
}

TEST_F(TemperatureReadingTest, JSONDeserialization) {
    nlohmann::json json_data = {
        {"spot_id", 2},
        {"temperature", 30.0},
        {"quality", "POOR"}
    };
    
    TemperatureReading reading;
    reading.from_json(json_data);
    
    EXPECT_EQ(reading.spot_id, 2);
    EXPECT_DOUBLE_EQ(reading.temperature, 30.0);
    EXPECT_EQ(reading.quality, ReadingQuality::POOR);
}

TEST_F(TemperatureReadingTest, AllQualityLevels) {
    std::vector<ReadingQuality> qualities = {
        ReadingQuality::GOOD,
        ReadingQuality::POOR,
        ReadingQuality::INVALID,
        ReadingQuality::ERROR
    };
    
    for (auto quality : qualities) {
        TemperatureReading reading;
        reading.spot_id = 1;
        reading.temperature = 25.5;
        reading.timestamp = test_timestamp;
        reading.quality = quality;
        
        if (quality == ReadingQuality::ERROR) {
            reading.error_code = 1001;
        }
        
        EXPECT_TRUE(reading.validate()) << "Quality level should be valid: " 
                                       << static_cast<int>(quality);
    }
}

} // namespace thermal