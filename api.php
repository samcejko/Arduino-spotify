<?php
$client_id = 'YOUR_CLIENT_ID';
$client_secret = 'YOUR_CLIENT_SECRET';
error_reporting(0);

function cleanText($str)
{
    if (empty($str))
        return " ";
    $table = array(
        'Á' => 'A',
        'Ä' => 'A',
        'Č' => 'C',
        'Ď' => 'D',
        'É' => 'E',
        'Ě' => 'E',
        'Í' => 'I',
        'Ĺ' => 'L',
        'Ľ' => 'L',
        'Ň' => 'N',
        'Ó' => 'O',
        'Ô' => 'O',
        'Ö' => 'O',
        'Ř' => 'R',
        'Ŕ' => 'R',
        'Š' => 'S',
        'Ť' => 'T',
        'Ú' => 'U',
        'Ů' => 'U',
        'Ü' => 'U',
        'Ý' => 'Y',
        'Ž' => 'Z',
        'á' => 'a',
        'ä' => 'a',
        'č' => 'c',
        'ď' => 'd',
        'é' => 'e',
        'ě' => 'e',
        'í' => 'i',
        'ĺ' => 'l',
        'ľ' => 'l',
        'ň' => 'n',
        'ó' => 'o',
        'ô' => 'o',
        'ö' => 'o',
        'ř' => 'r',
        'ŕ' => 'r',
        'š' => 's',
        'ť' => 't',
        'ú' => 'u',
        'ů' => 'u',
        'ü' => 'u',
        'ý' => 'y',
        'ž' => 'z',
    );
    $str = strtr($str, $table);
    return preg_replace('/[^a-zA-Z0-9\s\(\)\-\.\,\&]/', '', $str);
}

$creds = @json_decode(@file_get_contents('creds.json'), true);
if (!is_array($creds))
    $creds = [];

if (!isset($creds['access_token']) || (time() > @$creds['created_at'] + 3500)) {
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, 'https://accounts.spotify.com/api/token');
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query([
        'grant_type' => 'refresh_token',
        'refresh_token' => $creds['refresh_token'] ?? '',
        'client_id' => $client_id,
        'client_secret' => $client_secret,
    ]));
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    $response = json_decode(curl_exec($ch), true);
    curl_close($ch);
    if (isset($response['access_token'])) {
        $creds['access_token'] = $response['access_token'];
        if (isset($response['refresh_token']))
            $creds['refresh_token'] = $response['refresh_token'];
        $creds['created_at'] = time();
        file_put_contents('creds.json', json_encode($creds));
    }
}

$ch = curl_init();
curl_setopt($ch, CURLOPT_URL, 'https://api.spotify.com/v1/me/player/currently-playing');
curl_setopt($ch, CURLOPT_HTTPHEADER, ["Authorization: Bearer " . ($creds['access_token'] ?? '')]);
curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
curl_setopt($ch, CURLOPT_TIMEOUT, 3);
$raw = curl_exec($ch);
$http_code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
curl_close($ch);

header('Content-Type: text/plain');

if ($http_code == 204 || empty($raw)) {
    echo "0\nPaused\nSpotify\n0\n-\n-\n";
    exit;
}

$data = json_decode($raw, true);

if ($data && isset($data['item'])) {
    $currentTrackID = $data['item']['id'];
    $lastTrackID = @file_get_contents('last_track_id.txt');

    echo "1\n";
    echo cleanText($data['item']['name']) . "\n";
    echo cleanText($data['item']['artists'][0]['name']) . "\n";

    $progress = $data['progress_ms'] ?? 0;
    $duration = $data['item']['duration_ms'] ?? 1;
    echo ($duration > 0 ? round(($progress / $duration) * 100) : 0) . "\n";

    $ch2 = curl_init();
    curl_setopt($ch2, CURLOPT_URL, 'https://api.spotify.com/v1/me/player/queue');
    curl_setopt($ch2, CURLOPT_HTTPHEADER, ["Authorization: Bearer " . $creds['access_token']]);
    curl_setopt($ch2, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch2, CURLOPT_TIMEOUT, 2);
    $queueRaw = curl_exec($ch2);
    curl_close($ch2);
    $qData = json_decode($queueRaw, true);

    if ($qData && isset($qData['queue'][0])) {
        echo cleanText($qData['queue'][0]['name']) . "\n";
        echo cleanText($qData['queue'][0]['artists'][0]['name']) . "\n";
    } else {
        echo "-\n-\n";
    }

    if ($currentTrackID === $lastTrackID && file_exists('cache_image.txt')) {
        readfile('cache_image.txt');
    } else {
        $hex = "";
        $url = $data['item']['album']['images'][1]['url'] ?? $data['item']['album']['images'][0]['url'];
        if ($url) {
            $src = @imagecreatefromjpeg($url);
            if ($src) {
                $dst = imagecreatetruecolor(200, 200);
                imagecopyresampled($dst, $src, 0, 0, 0, 0, 200, 200, imagesx($src), imagesy($src));
                imagefilter($dst, IMG_FILTER_GRAYSCALE);
                imagefilter($dst, IMG_FILTER_CONTRAST, -20);

                for ($y = 0; $y < 200; $y++) {
                    for ($x = 0; $x < 200; $x++) {
                        $rgb = imagecolorat($dst, $x, $y);
                        $gray = ($rgb >> 16) & 0xFF;
                        $new = ($gray > 120) ? 255 : 0;
                        $err = $gray - $new;
                        imagesetpixel($dst, $x, $y, imagecolorallocate($dst, $new, $new, $new));
                        distErr($dst, $x + 1, $y, $err * 7 / 16);
                        distErr($dst, $x - 1, $y + 1, $err * 3 / 16);
                        distErr($dst, $x, $y + 1, $err * 5 / 16);
                        distErr($dst, $x + 1, $y + 1, $err * 1 / 16);
                    }
                }

                $byte = 0;
                $cnt = 0;
                for ($y = 0; $y < 200; $y++) {
                    for ($x = 0; $x < 200; $x++) {
                        if ((imagecolorat($dst, $x, $y) & 0xFF) < 128)
                            $byte |= (1 << (7 - $cnt));
                        if (++$cnt == 8) {
                            $hex .= sprintf("%02X", $byte);
                            $byte = 0;
                            $cnt = 0;
                        }
                    }
                }
                if ($cnt > 0)
                    $hex .= sprintf("%02X", $byte);
                imagedestroy($src);
                imagedestroy($dst);
            } else {
                $hex = str_repeat("FF", 5000);
            }
        }
        file_put_contents('cache_image.txt', $hex);
        file_put_contents('last_track_id.txt', $currentTrackID);
        echo $hex;
    }
} else {
    echo "0\nPaused\nSpotify\n0\n-\n-\n";
}

function distErr($img, $x, $y, $err)
{
    if ($x >= 0 && $x < 200 && $y >= 0 && $y < 200) {
        $gray = (imagecolorat($img, $x, $y) >> 16) & 0xFF;
        $val = max(0, min(255, $gray + $err));
        imagesetpixel($img, $x, $y, imagecolorallocate($img, $val, $val, $val));
    }
}
?>