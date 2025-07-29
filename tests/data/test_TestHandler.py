
def test_parse_output():
    # Arrange
    mock_runwindow = MagicMock()
    mock_master = MagicMock()
    test_handler = TestHandler(mock_runwindow, mock_master, info="SomeInfo", firmware=[], txt_files={})

    


    match = re.search(r'Configuring chips of hybrid: (\d+)$', line)

    # Assert - example assertion for output file
    assert test_handler.mod_dict["A"]["1"] == "5.0"
    assert test_handler.mod_dict["A"]["1"] == "1234"

if __name__ == "__main__":
    import re

    line = "|06:25:07|I|Configuring chips of hybrid: 0"
    match = re.search(r'Configuring chips of hybrid: (\d+)$', line)

    if match:
        print(match.group(1))  # Output: 0
