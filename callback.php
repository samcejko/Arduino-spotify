<?php
$client_id = 'YOUR_CLIENT_ID';
$client_secret = 'YOUR_CLIENT_SECRET';
$redirect_uri = 'YOUR_REDIRECT_URI'; // https://twojedomena.cz/spotify/callback.php
$scope = 'user-read-playback-state user-read-currently-playing';

if (isset($_GET['code'])) {
    $code = $_GET['code'];
    $ch = curl_init();
    curl_setopt($ch, CURLOPT_URL, 'https://accounts.spotify.com/api/token');
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, http_build_query([
        'grant_type' => 'authorization_code',
        'code' => $code,
        'redirect_uri' => $redirect_uri,
        'client_id' => $client_id,
        'client_secret' => $client_secret,
    ]));
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    $response = curl_exec($ch);
    curl_close($ch);

    file_put_contents('creds.json', $response);
    echo "HOTOVO! Tokeny uloženy. Teď můžeš nahrát kód do Arduina.";
} else {
    $url = "https://accounts.spotify.com/authorize?" . http_build_query([
        'response_type' => 'code',
        'client_id' => $client_id,
        'scope' => $scope,
        'redirect_uri' => $redirect_uri,
    ]);
    echo "<a href='$url'>PŘIHLÁSIT SE DO SPOTIFY</a>";
}
?>