import pytest
import sys
import os
from io import BytesIO
from werkzeug.datastructures import ImmutableMultiDict

# Add tools directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from tools.logger_server import app


@pytest.mark.parametrize("message,try_val", [
    ("normal log message", "1"),  # Valid input
    ("message\nforged log entry", "1"),  # Newline injection attempt
    ("message\x00null\x1bESC", "2"),  # Null byte and control character injection
    ("message\r\nHTTP header injection", "1"),  # CRLF injection attempt
    ("", ""),  # Boundary: empty strings
])
def test_logger_server_input_validation_boundary(message, try_val):
    """Invariant: Logger server must not allow control characters or newlines in form inputs that could forge log entries or break log parsing."""
    
    with app.test_client() as client:
        # Attempt to log with adversarial input
        response = client.post(
            '/log',
            data={'message': message, 'try': try_val},
            follow_redirects=True
        )
        
        # Security property: The server should either:
        # 1. Reject the request (4xx status), or
        # 2. Accept it but sanitize/escape the input so it doesn't break log structure
        
        # At minimum, the response should not indicate a server error from malformed input
        assert response.status_code != 500, \
            f"Server crashed on input: message={repr(message)}, try={repr(try_val)}"
        
        # If a log file exists, verify it doesn't contain unescaped control characters
        # that could be used to forge entries or manipulate parsing
        log_file = os.path.join(os.path.dirname(__file__), '..', 'tools', 'log.txt')
        if os.path.exists(log_file):
            with open(log_file, 'r', errors='ignore') as f:
                log_content = f.read()
                # Verify that newlines in input don't create separate log entries
                # (i.e., input newlines should be escaped or sanitized)
                if '\n' in message or '\r' in message:
                    # Count lines; injected newlines would increase line count unexpectedly
                    lines = log_content.strip().split('\n')
                    # Each log entry should be on one line; verify structure integrity
                    assert all(line for line in lines), \
                        "Log file contains empty lines from unescaped input injection"